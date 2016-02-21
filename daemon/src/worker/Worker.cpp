//
// Created by e-al on 28.11.15.
//

#include <easylogging++.h>
#include <zmq.hpp>

#include <dlfcn.h>
#include <mutex>
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <thread>

#include <crane.pb.h>
#include <node/TopologyNodeBase.h>
#include <topology/Topology.h>
#include <src/message/CraneMessageParser.h>
#include <src/message/WorkerMessageHandler.h>
#include <tuple/Tuple.h>
#include <SystemHelper.h>
#include "Worker.h"


// Base class representing impl interface
struct Worker::Impl
{
    virtual void pushTuples(const std::list<Tuple *> &tuples) = 0;
    virtual void pushTuple(Tuple* tuple) = 0;
    virtual void start() = 0;
    virtual void kill() = 0;
    virtual void configureConnections(const std::list<ConnectionGroup>& connectionGroups) = 0;
    virtual void setDataPort(int dataPort) = 0;
    virtual void setId(const std::string& id) = 0;
    virtual void setLib(const std::string& lib) = 0;
    virtual void setBolt(bool isBolt) = 0;
    virtual void startProcessing()  {}
    virtual void test() {}
};

// This implementation will be running in child process
struct Worker::ChildImpl : public Worker::Impl
{
    struct InternalConnectionGroup
    {
        std::string childName;
        std::string grouping;
        std::vector<zmq::socket_t> connections;
    };

    using ConnectionGroups = std::list<UniquePointer<InternalConnectionGroup>>;

    ChildImpl(Worker *newParent, const std::string& newId, const std::string& newLib, int newPort, bool newIsBolt)
    : parent(newParent)
    , id(newId)
    , lib(newLib)
    , dataPort(newPort)
    , isBolt(newIsBolt)
    , context(1)
    , dataReceiveSocket(context, ZMQ_PULL)
    , dataSendSocket(context, ZMQ_PUSH)
    , commandSocket(context, ZMQ_REP)
    , connections()
    , messageHandler(newParent)
    , inData(std::make_shared<TupleStream>(60000))
    , outData(std::make_shared<TupleStream>(60000))
    {
    }

    CraneWrappedMessagePointer createTupleBatchMessage()
    {
//        TupleStream localStream;
//        {
//            std::unique_lock<std::mutex> lk(outDataMutex);
//            outData->swap(localStream);
//        }

        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::TUPLE_BATCH);
//        while (!localStream.empty())
        while (!outData->empty())
        {
//            auto tuple = localStream.front();
//            auto tuple = std::make_shared<Tuple>();
            Tuple *tuple;
//            localStream.pop();
            outData->pop(tuple);
            auto craneTuple = msg->mutable_cranetuplebatch()->add_tuples();
            craneTuple->set_id(tuple->id);

            for (const auto& dataString : tuple->data) { craneTuple->add_data(dataString); }
            delete tuple;
        }

        return msg;
    }

    CraneWrappedMessagePointer createTupleBatchMessage(const std::list<Tuple *> stream)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::TUPLE_BATCH);
        for (const auto& tuple : stream)
        {
            auto craneTuple = msg->mutable_cranetuplebatch()->add_tuples();
            craneTuple->set_id(tuple->id);
            for (const auto& dataString : tuple->data) { craneTuple->add_data(dataString); }
        }

        return msg;
    }

    CraneWrappedMessagePointer createTupleMessage(const crane::CraneTuple& tuple)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::TUPLE);

        for (const auto& field : tuple.data()) { msg->mutable_cranetuple()->add_data(field); }
        msg->mutable_cranetuple()->set_id(tuple.id());

        return msg;
    }

    CraneWrappedMessagePointer createTupleMessage(const Tuple* tuple)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::TUPLE);

        for (const auto& field : tuple->data) { msg->mutable_cranetuple()->add_data(field); }
        msg->mutable_cranetuple()->set_id(tuple->id);

        return msg;
    }

    void dataReceiveThread()
    {
        dataReceiveSocket.bind(SystemHelper::makeAddr("*", dataPort).c_str());
        zmq::message_t msg;

        while (true)
        {
            try
            {
                dataReceiveSocket.recv(&msg);
                CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
        }
    }

    void readAllTuples(std::list<Tuple *>& out)
    {
        while (!outData->empty())
        {
            Tuple *tuple;
            outData->pop(tuple);
            out.push_back(tuple);
        }
    }

    void dataSendThread()
    {

        zmq::message_t msg;

        while (true)
        {
            {
                if (outData->empty())
                {
//                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
            }

            std::list<Tuple *> localStream;
            readAllTuples(localStream);
            std::cout << "Batch size: " << localStream.size() << std::endl;
            {
                std::unique_lock<std::mutex> lk(connectionsMutex);
                for (auto& connectionGroup : connectionGroups)
                {
                    auto groupingScheme = topology::Node::parseGroupingString(connectionGroup->grouping);
                    switch (groupingScheme)
                    {
                        case topology::GroupingScheme::SHUFFLE:
                        {
                            auto tupleBatchMessage = createTupleBatchMessage(localStream);
                            auto connectionIt = SystemHelper::selectRandom(connectionGroup->connections.begin()
                                                                           , connectionGroup->connections.end());

                            msg.rebuild(tupleBatchMessage->ByteSize());
                            tupleBatchMessage->SerializeToArray(msg.data(), tupleBatchMessage->ByteSize());

                            try
                            {
                                connectionIt->send(msg);
                            }
                            catch (const std::logic_error &e)
                            {
                                LOG(ERROR) << e.what();
                            }

                            break;
                        }

                        case topology::GroupingScheme::FIELD:
                        {
                            std::map<int, CraneWrappedMessagePointer> messages;
                            for (const auto& tuple : localStream)
                            {
                                auto hashedField = SystemHelper::hashField(tuple->data[0]);


                                auto dest = hashedField % connectionGroup->connections.size();

                                if (messages.find(dest) == messages.end())
                                {
                                    messages[dest] = std::make_shared<crane::CraneWrappedMessage>();
                                    messages[dest]->set_type(crane::TUPLE_BATCH);
                                }

                                auto newTuple = messages[dest]->mutable_cranetuplebatch()->add_tuples();
                                newTuple->set_id(tuple->id);
                                for (const auto& dataField : tuple->data)
                                {
                                    newTuple->add_data(dataField);
                                }
                            }

                            for (const auto& kv : messages)
                            {
                                auto &socket = connectionGroup->connections[kv.first];
                                const auto& tupleMsg = kv.second;
                                msg.rebuild(tupleMsg->ByteSize());
                                tupleMsg->SerializeToArray(msg.data(), tupleMsg->ByteSize());

                                try
                                {
                                    socket.send(msg);
                                }
                                catch (const std::logic_error &e)
                                {
                                    LOG(ERROR) << e.what();
                                }
                            }
                            break;
                        }
                    }
                }
            }

            for (auto& tuple : localStream) { delete tuple; }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void dataProcessThread()
    {
        void *handle = dlopen(lib.c_str(), RTLD_LAZY);

        using Constructor = TopologyNodeBase * (*)();
        using Destructor = void (*)(TopologyNodeBase *);

        Constructor create;
        Destructor destroy;

        create = (Constructor)(dlsym(handle, "create_object"));
        destroy = (Destructor)(dlsym(handle, "destroy_object"));

        // create bolt/spout class
        TopologyNodeBase *node = static_cast<TopologyNodeBase *>(create());

        // enter tuple processing loop
        if (isBolt)
        {
            while (true)
            {
                std::queue<Tuple *> localStream;
                {
                    if (inData->empty())
                    {
                        continue;
                    }
                    while (!inData->empty())
                    {
                        Tuple *tuple;
                        inData->pop(tuple);
                        localStream.push(tuple);
                    }
                }

                {
                    while (!localStream.empty())
                    {
                        node->runOnTuple(localStream.front(), outData);
                        localStream.pop();
                    }
                }

            }
        }
            // This is spout
        else
        {
            SpoutBase *spout = static_cast<SpoutBase *>(node);
            int counter = 0;
            while (true)
            {
                {
                    Tuple *tuple = spout->nextTuple();
                    outData->push(tuple);
                    if (++counter > 5000)
                    {
                        counter = 0;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                    }
                }
            }

        }

    }

    void commandThread()
    {
        commandSocket.bind(id.c_str());
        zmq::message_t msg;

        while (true)
        {
            try
            {
                commandSocket.recv(&msg);
                CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
        }
    }

    virtual void start()
    {
        // Enter command processing loop
        commandThread();
    }

    void startProcessing() override
    {
        std::thread dataSendingThread(&ChildImpl::dataSendThread, this);
        std::thread dataReceivingThread(&ChildImpl::dataReceiveThread, this);
        std::thread dataProcessingThread(&ChildImpl::dataProcessThread, this);
//        std::thread commandProcessingThread(&ChildImpl::commandThread, this);

        dataSendingThread.detach();
        dataReceivingThread.detach();
        dataProcessingThread.detach();
        replyToSupervisor(crane::CraneSupervisorReply::START);
        // need to join commandProcessingThread to prevent the worker executable from exiting main()
//        commandProcessingThread.join();
    }

    virtual void kill()
    {
        replyToSupervisor(crane::CraneSupervisorReply::KILL);
        exit(0);
    }


    virtual void pushTuple(Tuple* tuple) override
    {
        inData->push(tuple);
    }

    virtual void pushTuples(const std::list<Tuple *> &tuples) override
    {
        for (const auto& tuple : tuples) { inData->push(tuple); }
    }

    virtual void configureConnections(const std::list<ConnectionGroup>& newConnectionGroups) override
    {
//        std::cout << "Try to configure connections on worker " << id << dataPort << std::endl;
        std::unique_lock<std::mutex> lk(connectionsMutex);
        // Close existing connections
        for (const auto& connectionGroup : connectionGroups)
        {
            for (auto& socket : connectionGroup->connections) { socket.close(); }
        }

        // And clear them
        connectionGroups.clear();

        for (const auto& connectionGroup : newConnectionGroups)
        {

            UniquePointer<InternalConnectionGroup> newConnectionGroup{new InternalConnectionGroup};
            newConnectionGroup->childName = connectionGroup.childName;
            newConnectionGroup->grouping = connectionGroup.grouping;

            for (const auto& address : connectionGroup.addresses)
            {
                newConnectionGroup->connections.push_back(zmq::socket_t(context, ZMQ_PUSH));
                newConnectionGroup->connections.back().connect(address.c_str());
            }

            connectionGroups.push_back(std::move(newConnectionGroup));
        }
        replyToSupervisor(crane::CraneSupervisorReply::CONFIG_CONNS);
    }

    CraneWrappedMessagePointer createReplyMessage(crane::CraneSupervisorReply::SupervisorReplyType replyType)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::SUPERVISOR_REP);
        msg->mutable_cranesupervisorreply()->set_type(replyType);

        return msg;
    }

    // Since all the replies are trivial we just need a reply type to construct the message
    void replyToSupervisor(crane::CraneSupervisorReply::SupervisorReplyType replyType)
    {
        zmq::message_t msg;
        auto replyMessage = createReplyMessage(replyType);
        msg.rebuild(replyMessage->ByteSize());
        replyMessage->SerializeToArray(msg.data(), replyMessage->ByteSize());
        try
        {
            commandSocket.send(msg);
        }
        catch (const std::logic_error &e)
        {
            LOG(ERROR) << e.what();
        }
    }


    virtual void setDataPort(int dataPort) { this->dataPort = dataPort; }
    virtual void setId(const std::string &id) { this->id = id; }
    virtual void setLib(const std::string &lib) { this->lib = lib; }
    virtual void setBolt(bool isBolt) { this->isBolt = isBolt; }
    void test() override { std::cout << "Worker::ChildImpl is okay" << std::endl; }

    Worker *parent;
    std::string id;
    std::string lib;
    int dataPort;
    bool isBolt;
    zmq::context_t context;
    zmq::socket_t dataReceiveSocket;
    zmq::socket_t dataSendSocket;
    zmq::socket_t commandSocket;
    std::list<zmq::socket_t> connections;
    ConnectionGroups connectionGroups;
    WorkerMessageHandler messageHandler;

    TupleStreamPointer inData;
    TupleStreamPointer outData;
    std::mutex inDataMutex;
    std::mutex outDataMutex;
    std::mutex connectionsMutex;

};

// This implementation will be running in parent process and act as a wrapper to communicate with child process
// For users this communication is transparent
struct Worker::ParentImpl : public Worker::Impl
{

    ParentImpl(Worker *newParent, const std::string& newId, const std::string& newLib, int newPort, bool newIsBolt)
    : parent(newParent)
    , id(newId)
    , lib(newLib)
    , dataPort(newPort)
    , isBolt(newIsBolt)
    , context(1)
    , commandSocket(context, ZMQ_REQ)
    , messageHandler(newParent)
    {
        std::cout << "Worker " << id << " created as wrapper" << std::endl;
        std::string address = id + std::to_string(dataPort);
        commandSocket.connect(address.c_str());
    }

    virtual void pushTuple(Tuple *tuple) override
    {
        // In parent implementation we are not supposed to call this method
        assert("Cannot push tuples to a worker directly!");
    }

    virtual void pushTuples(const std::list<Tuple *> &tuples) override
    {
        // In parent implementation we are not supposed to call this method
        assert("Cannot push tuples to a worker directly!");
    }

    virtual void start() override
    {
        std::cout << "Try to start on PARENT worker " << id << dataPort << std::endl;
        auto startMessage = createStartMessage();
        requestAndProcessReply(startMessage);
        std::cout << "Started on PARENT worker " << id << dataPort << std::endl;
    }

    virtual void kill() override
    {
        auto killMessage = createKillMessage();
        requestNoReply(killMessage);
    }

    virtual void configureConnections(const std::list<ConnectionGroup>& connectionGroups) override
    {
//        std::cout << "Try to configure connections on PARENT worker " << id << dataPort << std::endl;
        auto configureConnectionsMessage = createConfigureConnectionsMessage(connectionGroups);
        requestAndProcessReply(configureConnectionsMessage);
//        std::cout << "Connections configured on PARENT worker" << id << dataPort << std::endl;
    }

    CraneWrappedMessagePointer createConfigureConnectionsMessage(
            const std::list<ConnectionGroup>& connectionGroups)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::SUPERVISOR_REQ);
        msg->mutable_cranesupervisorrequest()->set_type(crane::CraneSupervisorRequest::CONFIG_CONNS);
        auto configureConnectionsMessage
                = msg->mutable_cranesupervisorrequest()->mutable_configureconnectionsrequest();
        for (const auto& group : connectionGroups)
        {
            auto newConnectionGroup = configureConnectionsMessage->add_configureconnectionsrequest();
            newConnectionGroup->set_childname(group.childName);
            newConnectionGroup->set_grouping(group.grouping);
            for (const auto& address : group.addresses) { newConnectionGroup->add_addresses(address); }
        }

        return msg;
    }

    CraneWrappedMessagePointer createStartMessage()
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::SUPERVISOR_REQ);
        msg->mutable_cranesupervisorrequest()->set_type(crane::CraneSupervisorRequest::START);

        return msg;
    }

    CraneWrappedMessagePointer createKillMessage()
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::SUPERVISOR_REQ);
        msg->mutable_cranesupervisorrequest()->set_type(crane::CraneSupervisorRequest::KILL);

        return msg;
    }

    void requestNoReply(const CraneWrappedMessagePointer& message)
    {
        zmq::message_t msg;
        msg.rebuild(message->ByteSize());
        message->SerializeToArray(msg.data(), message->ByteSize());
        try
        {
            commandSocket.send(msg);
        }
        catch (const std::logic_error &e)
        {
            LOG(ERROR) << e.what();
        }
    }
    void requestAndProcessReply(const CraneWrappedMessagePointer& message)
    {
        zmq::message_t msg;
        msg.rebuild(message->ByteSize());
        message->SerializeToArray(msg.data(), message->ByteSize());
        try
        {
            commandSocket.send(msg);
            msg.rebuild();
            commandSocket.recv(&msg);
            CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::logic_error &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    virtual void setDataPort(int dataPort) { this->dataPort = dataPort; }
    virtual void setId(const std::string &id) { this->id = id; }
    virtual void setLib(const std::string &lib) { this->lib = lib; }
    virtual void setBolt(bool isBolt) { this->isBolt = isBolt; }

    Worker *parent;
    std::string id;
    std::string lib;
    int dataPort;
    bool isBolt;
    zmq::context_t context;
    zmq::socket_t commandSocket;
    WorkerMessageHandler messageHandler;

};

Worker::Worker(const std::string &id, int dataPort, const std::string &lib, bool isBolt, bool asWrapper)
{
    std::cout << __PRETTY_FUNCTION__ << ": id == " << id << std::endl;
    if (asWrapper)
    {
        _pimpl = std::move(UniquePointer<Impl>(new ParentImpl(this, id, lib, dataPort, isBolt)));
    }
    else
    {
        _pimpl = std::move(UniquePointer<Impl>(new ChildImpl(this, id, lib, dataPort, isBolt)));
    }

    _pimpl->setDataPort(dataPort);
    _pimpl->setId(id);
    _pimpl->setLib(lib);
    _pimpl->setBolt(isBolt);
}

void Worker::start()
{
    std::cout << "Calling Worker::start()" << std::endl;
    _pimpl->start();
}

void Worker::kill()
{
    _pimpl->kill();
}

void Worker::pushTuple(Tuple* tuple)
{
    _pimpl->pushTuple(tuple);
}

void Worker::pushTuples(const std::list<Tuple *> &tuples)
{
    _pimpl->pushTuples(tuples);
}

void Worker::configureConnections(const std::list<ConnectionGroup>& connectionGroups)
{
    _pimpl->configureConnections(connectionGroups);
}

Worker::~Worker()
{

}

void Worker::test()
{
    std::cout << "Impl: " << _pimpl.get() << std::endl;
    _pimpl->test();
}

void Worker::startProcessing()
{
    _pimpl->startProcessing();
}
