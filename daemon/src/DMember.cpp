//
// Created by Tianyuan Liu on 9/16/15.
//

#include <zmq.hpp>
#include <zmq_utils.h>

#include <easylogging++.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <queue>

#include <SystemHelper.h>

#include <event/NodeFailEvent.h>
#include <member/Member.h>
#include <message/MessageHandler.h>
#include <message/MessageParser.h>

#include "DMember.h"

using MembersQueue = std::queue<MemberMapPointer>;

struct DMember::Impl
{
    Impl(DMember *newparent)
        : parent(newparent)
        , context(1)
        , inSocket(nullptr)
        , outSocket(context, ZMQ_PUB)
        , pingSocket(context, ZMQ_REP)
    {
        setNodeId();
    }

    void addListener(int id, SimpleEventHandlerPointer handler)
    {
        eventHandlers[id].push_back(handler);
    }


    // Functions for initialization
    void setNodeId()
    {
        const std::string IPPrefix = "fa15-cs425-g10-0";
        const std::string IPSuffix = ".cs.illinois.edu";
        int nodeId = (SystemHelper::exec("uname -n")[static_cast<int>(IPPrefix.length())] - '0'); // get the vmid for current machine

        std::stringstream ss;
        ss << IPPrefix << nodeId << IPSuffix;
        id = ss.str();
    }

    void setupStaticTestbed(int total_nodes=7, int max_fail=2)
    {
        const std::string IPPrefix = "fa15-cs425-g10-0";
        const std::string IPSuffix = ".cs.illinois.edu";

        int nodeId = (SystemHelper::exec("uname -n")[static_cast<int>(IPPrefix.length())] - '0'); // get the vmid for current machine


        std::stringstream ss;
        ss << "tcp://*:" << std::to_string(heartbeatPort);
        outSocket.bind(ss.str().c_str());
        for (int i=1; i<=max_fail+1; ++i)
        {
            int id = (nodeId + total_nodes - i) % total_nodes;
            if (id == 0) id += total_nodes;
            ss.str("");
            ss << "tcp://" << IPPrefix << std::to_string(id) << IPSuffix << std::to_string(heartbeatPort);
            while(true) {
                try {
                    inSocket->connect(ss.str().c_str());
                    break;
                }
                catch (...) {
                    std::cerr << "Unable to connect to server: " << id << std::endl;
                    zmq_sleep(2);
                }
            }


        }
        std::cout << "Static testbed constructed for node " << nodeId << std::endl;

    }

    // Functions for introducer service
    void pingThread()
    {
        pingSocket.bind(SystemHelper::makeAddr("*", pingPort).c_str());
        zmq::message_t msg;

        while (true)
        {
            try {
                pingSocket.recv(&msg);
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
            {
                std::unique_lock<std::mutex> lock(isStoppedMutex);
                if (isStopped) { break; }
            }
        }
    }

    void replyPingRequest()
    {
        try
        {
            WrappedMessagePointer wrappedMessage = std::make_shared<dmember::WrappedMessage>();
            wrappedMessage->set_type(dmember::PING_ACK);
            wrappedMessage->mutable_pingack()->set_id(id);
            wrappedMessage->mutable_pingack()->set_timestamp(joinTime.time_since_epoch().count());
            wrappedMessage->mutable_pingack()->set_count(memberMap[id]->count());

            zmq::message_t msg(wrappedMessage->ByteSize());
            wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());

            pingSocket.send(msg);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void updateConnection(const std::vector<std::string> &members, int max_fail=2)
    {

        int size = static_cast<int>(members.size());
        if (size == 1) return;

        std::unique_lock<std::mutex> lock;
        inSocket->close();
        inSocket = new zmq::socket_t(context, ZMQ_SUB);
        inSocket->setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);

        //always connect to introducer
        std::string introducerAddr = SystemHelper::makeAddr(introducerAddress, heartbeatPort);
        inSocket->connect(introducerAddr.c_str());
        LOG(INFO) << "Connection Update";

        if (size <= max_fail+1)
        {
            for(auto member : members)
            {
                if (member == id) continue;

                inSocket->connect(SystemHelper::makeAddr(member, heartbeatPort).c_str());
                LOG(INFO) << "connect heartbeat socket to "
                    << SystemHelper::makeAddr(member, heartbeatPort);
            }
            return;
        }

        // find itself in the list, and connect to the 3 predecessors
        int index;
        for (index = 0; index < size; ++index)
            if (members[index] == id) break;

        for (int i=1; i<=max_fail+1; ++i) {
            int target_index = (index - i + size) % size;
            inSocket->connect(SystemHelper::makeAddr(members[target_index], heartbeatPort).c_str());
            LOG(INFO) << "connect heartbeat socket to "
                << SystemHelper::makeAddr(members[target_index], heartbeatPort);
        }
    }

    void pushMembersToQueue(const MemberMapPointer& members)
    {
        membersQueue.push(members);
    }

    void updateMembers()
    {
        std::list<MemberMapPointer> memberMaps;
        {
            while (!membersQueue.empty())
            {
                memberMaps.push_back(membersQueue.front());
                membersQueue.pop();
            }
        }

        auto selfNode = memberMap[id];

        // First update, init ourselves
        if (selfNode == nullptr)
        {
            selfNode = std::make_shared<Member>(id, 0);
            memberMap[id] = selfNode;
        }

        selfNode->update(selfNode->count() + 1, std::chrono::high_resolution_clock::now(), false, isStopped);
        if (isStopped)
        {
            std::cout << "just made ourself a leaving node" << std::endl;
        }

        for (const auto& map : memberMaps)
        {
            for (const auto& kv : *map)
            {
                auto member = memberMap.find(kv.first);
                if (member != memberMap.end())
                {
                    memberMap[kv.first]->update(*kv.second);
                }
                else if (!kv.second->isLeaving())
                {
                    memberMap[kv.first] = kv.second;
                    memberMap[kv.first]->setTimestamp(std::chrono::high_resolution_clock::now());
                    LOG(INFO) << "Node with id = " << kv.first << " has joined the group!";
                }
            }
        }

        std::list<std::string> deleteCandidates;
        for (const auto& kv : memberMap)
        {
            if (memberMap[kv.first]->shouldBeCleanUp())
            {
                deleteCandidates.push_back(kv.first);
                handleEvent(event::NodeFailEvent(kv.first));
                LOG(INFO) << "Node with id = " << kv.first << " has failed!";
            }

        }

        for (const auto& key : deleteCandidates) { memberMap.erase(key); }
    }

    void joinGroup()
    {
        std::string introducerAddr = SystemHelper::makeAddr(introducerAddress, Introducer::servicePort());
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.connect(introducerAddr.c_str());

        WrappedMessagePointer wrappedMessage = std::make_shared<dmember::WrappedMessage>();
        wrappedMessage->set_type(dmember::JOIN_REQ);
        wrappedMessage->mutable_joinrequest()->set_id(id);

        zmq::message_t msg(wrappedMessage->ByteSize());
        wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());

        try {
            socket.send(msg);

            msg.rebuild();
            bool success = false;
            success = socket.recv(&msg);
            if (success)
            {
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }

        }
        catch ( const std::logic_error &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    WrappedMessagePointer createHeartbeatMessage()
    {
        WrappedMessagePointer msg = std::make_shared<dmember::WrappedMessage>();
        msg->set_type(dmember::HEARTBEAT);

        std::list<std::string> leavingNodes;
        for (const auto& kv : memberMap)
        {
            if (memberMap[kv.first]->isLeaving())
            {
                auto memberList = msg->mutable_heartbeat()->add_memberlist();
                memberList->set_id(kv.first);
                memberList->set_count(kv.second->count());
                memberList->set_timestamp(kv.second->timestamp().time_since_epoch().count());
                memberList->set_leaving(kv.second->isLeaving());

                leavingNodes.push_back(kv.first);
                LOG(INFO) << "Node with id = " << kv.first << " is leaving!";
            }
            else if (!memberMap[kv.first]->isTimedOut())
            {
                auto memberList = msg->mutable_heartbeat()->add_memberlist();
                memberList->set_id(kv.first);
                memberList->set_count(kv.second->count());
                memberList->set_timestamp(kv.second->timestamp().time_since_epoch().count());
                memberList->set_leaving(kv.second->isLeaving());
            }
            else
            {
                LOG(INFO) << "Node with id = " << kv.first << " is suspicious!";
            }
        }

        for (const auto& node : leavingNodes)
        {
            memberMap.erase(node);
        }

        msg->mutable_heartbeat()->set_updateconnection(updateOnNextHeartbeat);
        updateOnNextHeartbeat = false;

        return msg;
    }

    // We connect to an introducer to listen to it all the time
    // We then join the group, waiting for a heartbeat that we will use to organize our connections
    void joinAndListenThread()
    {
        std::string introducerAddr = SystemHelper::makeAddr(introducerAddress, heartbeatPort);
        inSocket = new zmq::socket_t(context, ZMQ_SUB);
        inSocket->setsockopt(ZMQ_SUBSCRIBE, nullptr, 0);
        inSocket->connect(introducerAddr.c_str());

        joinGroup();

        zmq::message_t msg;

        while (true)
        {
            try
            {
                inSocket->recv(&msg);
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::logic_error &e)
            {
                LOG(ERROR) << e.what();
            }
            {
                std::unique_lock<std::mutex> lock(isStoppedMutex);
                if (isStopped) { break; }
            }
        }
    }

    void heartbeatsSendThread()
    {
        zmq::message_t msg;
        outSocket.bind(SystemHelper::makeAddr("*", heartbeatPort).c_str());

        while (true)
        {
            {
                std::unique_lock<std::mutex> lockQueue(memberQueueMutex);
                std::unique_lock<std::mutex> lockStopped(isStoppedMutex);
                updateMembers();
                try
                {
                    auto heartbeatMessage = createHeartbeatMessage();
                    msg.rebuild(heartbeatMessage->ByteSize());
                    heartbeatMessage->SerializeToArray(msg.data(), heartbeatMessage->ByteSize());

                    outSocket.send(msg);
                    if (isStopped)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        std::cout << "gracefully leaving" << std::endl;
                        senderFinished = true;
                        finishCondVar.notify_one();
                        break;
                    }
                }
                catch (const std::logic_error &e)
                {
                    LOG(ERROR) << e.what();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    // start member function
    void start()
    {
        std::thread pingingThread(&Impl::pingThread, this);
        std::thread hearbeatsRcvThread(&Impl::joinAndListenThread, this);
        std::thread hearbeatsSndThread(&Impl::heartbeatsSendThread, this);

        pingingThread.detach();
        hearbeatsRcvThread.detach();
        hearbeatsSndThread.detach();
    }


    // test-suite functions
    void sendTestMsg()
    {
        zmq::message_t msg;
        memcpy((void *) msg.data (), "Hello", 5);
        outSocket.send(msg);
        std::cout << "Hello sent" << std::endl;
        return;
    }

    void recvTestMsg()
    {
        zmq::message_t msg;
        inSocket->recv(&msg);
        std::cout << "Hello received" << std::endl;
        return;
    }

    template<typename EventT>
    void handleEvent(const EventT& event)
    {
        auto handlers = eventHandlers.find(event::getId<EventT>());
        if (handlers != eventHandlers.end())
        {
            for (const auto& handler : handlers->second)
            {
                std::cout << "Calling handler for NodeFailureEvent" << std::endl;
                handler->handle(event);
            }
        }
    }

    DMember *parent;
    zmq::context_t context;
    zmq::socket_t* inSocket;
    zmq::socket_t outSocket;
    zmq::socket_t pingSocket;
    MessageHandler messageHandler;

    // Sync access to isStopped variable, through which we notify threads to stop execution
    std::mutex isStoppedMutex;
    // Sync access to member queue
    std::mutex memberQueueMutex;
    // Sync access to condvar finishCondVar, through which sender thread notifies its completeion to the main thread
    std::mutex finishMutex;
    std::condition_variable finishCondVar;

    std::string id;
    TimeStamp joinTime;
    MemberMap memberMap;
    MembersQueue membersQueue;
    bool updateOnNextHeartbeat = false;
    bool isStopped = false;
    bool senderFinished = false;

    const int memberPort = 9876;
    const int pingPort = 9879;
    const int heartbeatPort = 9878;
    const std::string introducerAddress = "fa15-cs425-g10-01.cs.illinois.edu";
    std::map<int, EventHandlers> eventHandlers;
};

// Initialization
DMember::DMember()
    : _pImpl(new Impl(this))
{
    // uncomment the following line to test code with static setup
    //_pImpl->setupStaticTestbed();
}

DMember* DMember::instance()
{
    static DMember *app = nullptr;
    if (app == nullptr) app = new DMember();
    return app;
}

void DMember::start(bool asIntroducer)
{
    if (asIntroducer)
    {
        introducer = new Introducer(this);
        introducer->start();
    }

    zmq_sleep(1);
    _pImpl->start();
}


// public APIs
void DMember::updateJoinTime(long time)
{
    _pImpl->joinTime = TimeStamp() + std::chrono::nanoseconds(time);
}

void DMember::replyPingRequest()
{
    _pImpl->replyPingRequest();
}

void DMember::updateConnection(const std::vector<std::string> &members)
{
    _pImpl->updateConnection(members);
}

void DMember::updateMembers(const MemberMapPointer &members)
{
    std::unique_lock<std::mutex> lock(_pImpl->memberQueueMutex);
    _pImpl->pushMembersToQueue(members);
}

DMember::~DMember()
{

}

void DMember::updateConnectionsOnNextHeartbeat(const MemberMapPointer &members)
{
    std::unique_lock<std::mutex> lock(_pImpl->memberQueueMutex);
    _pImpl->updateOnNextHeartbeat = true;
    _pImpl->pushMembersToQueue(members);
}

int DMember::pingPort()
{
    return _pImpl->pingPort;
}

void DMember::stop()
{
    {
        std::unique_lock<std::mutex> lock(_pImpl->isStoppedMutex);
        _pImpl->isStopped = true;
    }

    {
        std::unique_lock<std::mutex> lock(_pImpl->finishMutex);
        _pImpl->finishCondVar.wait(lock, [this] { return _pImpl->senderFinished; });
    }
}

bool DMember::checkIfLeaving(const MemberId &id)
{
    auto member = _pImpl->memberMap.find(id);
    if (member == _pImpl->memberMap.end())
    {
        return true;
    }
    return member->second->isLeaving();
}

std::string DMember::joinTime()
{
    return SystemHelper::timeToString(_pImpl->joinTime);
}

std::string DMember::id()
{
    return _pImpl->id;
}

std::list<std::string> DMember::membershipList()
{
    std::list<std::string> list;
    for (const auto& kv : _pImpl->memberMap)
    {
        list.push_back(kv.second->id());
    }

    return list;
}

std::list<std::string> DMember::membershipListInfo()
{
    std::list<std::string> list;
    for (const auto& kv : _pImpl->memberMap)
    {
        list.push_back(kv.second->toString());
    }

    return list;
}
void DMember::addListener(int eventId, SimpleEventHandlerPointer handler)
{
    _pImpl->addListener(eventId, handler);
}

