//
// Created by e-al on 01.11.15.
//

#include <easylogging++.h>

#include <zmq.hpp>

#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <DMember.h>
#include <message/ElectionMessageHandler.h>
#include <message/ElectionMessageParser.h>
#include <SystemHelper.h>
#include <event/LeaderElectedEvent.h>
#include <event/NodeFailEvent.h>
#include "Candidate.h"


using SendToNodeFunction = std::function<void(const std::string&)>;

struct Candidate::Impl
{
    Impl(Candidate *newParent)
        : parent(newParent)
        , context(1)
        , inSocket(context, ZMQ_REP)
        , messageHandler()
        , leaderId("")
    {
        DMember::instance()->subscribe<event::NodeFailEvent>(parent);
    }

    void electionThread()
    {
        // Wait until we get all the members
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        {
            std::unique_lock<std::mutex> lk(startedMutex);
            if (started) { return; }

            started = true;
        }
        // Return if protocol is already running
//        setLeader("");

        auto ownId = DMember::instance()->id();

        auto membershipList = DMember::instance()->membershipList();

        // Wait until DMember updates its membership list
        while (!membershipList.size())
        {
            membershipList = DMember::instance()->membershipList();
        }

        auto min = std::min_element(membershipList.begin(), membershipList.end());

        // if our id is the lowest - elect ourselves as a leader
        if (*min == ownId)
        {
            exptectedConfirmationCount = membershipList.size();
            receivedConfirmationCount = 0;
            sendElected(membershipList);
        }
        // send request message to nodes with lower id
        else
        {
            std::list<std::string> lowerIds = SystemHelper::lowerIds(membershipList, ownId);
            sendElectionStart(lowerIds);
        }
    }

    void sendElected(const std::list<std::string> nodes)
    {
        SendToNodeFunction function = std::bind(&Impl::sendElectedThread, this, std::placeholders::_1);
        sendToNodes(nodes, function);
    }

    void sendElectedThread(const std::string &node)
    {
        const int timeout = 1000;
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        std::string nodeAddr = SystemHelper::makeAddr(node, requestPort);
        socket.connect(nodeAddr.c_str());

        ElectionWrappedMessagePointer wrappedMessage = std::make_shared<election::ElectionWrappedMessage>();
        wrappedMessage->set_type(election::NEW_LEADER);
        wrappedMessage->mutable_newleader()->set_id(DMember::instance()->id());

        zmq::message_t msg(wrappedMessage->ByteSize());
        wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());
        bool success = false;
        try {
            socket.send(msg);
            msg.rebuild();
            success = socket.recv(&msg);
            if (success) {
                ElectionMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }

        socket.close();
    }

    void replyOk()
    {
        try
        {
            ElectionWrappedMessagePointer wrappedMessage = std::make_shared<election::ElectionWrappedMessage>();
            wrappedMessage->set_type(election::OK_MSG);

            zmq::message_t msg(wrappedMessage->ByteSize());
            wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());

            inSocket.send(msg);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendElectionConfirmation()
    {
        try
        {
            ElectionWrappedMessagePointer wrappedMessage = std::make_shared<election::ElectionWrappedMessage>();
            wrappedMessage->set_type(election::ELECT_CONFIRM);

            zmq::message_t msg(wrappedMessage->ByteSize());
            wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());

            inSocket.send(msg);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void sendElectionStart(const std::list<std::string>& nodes)
    {
        okReceived = false;

        SendToNodeFunction function = std::bind(&Impl::requestElectionStartThread, this, std::placeholders::_1);
        sendToNodes(nodes, function, true);

        if (!okReceived)
        {
           // No one responded, name ourself a leader
//            std::cout << "No okays received, claim outselves a leader" << std::endl;
            auto membershipList = DMember::instance()->membershipList();
            sendElected(membershipList);
        }
//        std::cout << "at least someone responded with okay" << std::endl;
//        else
//        {
//            // Start waiting for elected message
//            // If not received within timeout - restart election
//            std::thread checkElected([this]()
//                                     {
//                                         std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//                                         if (leader().length() == 0)
//                                         {
//                                             std::cout << "Restarting election" << std::endl;
//                                             electionThread();
//                                         }
//                                     });
//            checkElected.detach();
//        }
    }

    std::string leader()
    {
        std::unique_lock<std::mutex> lk(leaderMutex);
        return leaderId;
    }

    void setLeader(const std::string& leader)
    {
        std::unique_lock<std::mutex> lk(leaderMutex);
        leaderId = leader;
    }

    void serviceThread()
    {
        zmq::message_t msg;
        std::string serviceAddr = SystemHelper::makeAddr("*", requestPort);
        inSocket.bind(serviceAddr.c_str());
        while (true)
        {
            try
            {
                inSocket.recv(&msg);
                ElectionMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::logic_error &e)
            {
                LOG(ERROR) << e.what() << " in Candidate";
            }
        }
    }

    void start()
    {
        std::thread servThread(&Impl::serviceThread, this);
        std::thread electThread(&Impl::electionThread, this);

        servThread.detach();
        electThread.detach();
    }

    void requestElectionStartThread(const std::string &node)
    {
        const int timeout = 1000;
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        std::string nodeAddr = SystemHelper::makeAddr(node, requestPort);
        socket.connect(nodeAddr.c_str());

        ElectionWrappedMessagePointer wrappedMessage = std::make_shared<election::ElectionWrappedMessage>();
        wrappedMessage->set_type(election::ELECTION_START);

        zmq::message_t msg(wrappedMessage->ByteSize());
        wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());
        bool success = false;
        try {
            socket.send(msg);
            msg.rebuild();
            success = socket.recv(&msg);
            if (success) {
                ElectionMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }

        socket.close();
    }

    void setElectionFinished()
    {
        std::unique_lock<std::mutex> lk(startedMutex);
        started = false;
    }

    void sendToNodes(const std::list<std::string>& nodes, SendToNodeFunction function, bool join = false)
    {
        std::vector<std::thread> sendingThreads;
        for (const auto& node : nodes)
        {
            sendingThreads.push_back(std::thread(function, node));
        }

        for (auto& thread : sendingThreads) { join ? thread.join() : thread.detach(); }
    }

    void addListener(int id, SimpleEventHandlerPointer handler)
    {
        eventHandlers[id].push_back(handler);
    }

    template<typename EventT>
    void handleEvent(const EventT& event)
    {
        auto handlers = eventHandlers.find(event::getId<EventT>());
        if (handlers != eventHandlers.end())
        {
            for (const auto& handler : handlers->second)
            {
                handler->handle(event);
            }
        }
    }

    void onElectionConfirmationReceive()
    {
        std::unique_lock<std::mutex> lk(confirmationReceivedMutex);
        ++receivedConfirmationCount;
    }

    int confirmationCount()
    {
        std::unique_lock<std::mutex> lk(confirmationReceivedMutex);
        return receivedConfirmationCount;
    }

    Candidate *parent;
    zmq::context_t context;
    zmq::socket_t inSocket;
    ElectionMessageHandler messageHandler;

    std::string leaderId;
    std::string previousLeaderId;
    bool started = false;
    bool okReceived = false;

    std::mutex startedMutex;
    std::mutex confirmationReceivedMutex;
    std::mutex leaderMutex;

    std::map<int, EventHandlers> eventHandlers;

    int exptectedConfirmationCount = 0;
    int receivedConfirmationCount = 0;
    const int requestPort = 7777;
};

Candidate::Candidate()
    : _pimpl(new Impl(this))
{

}

std::string Candidate::leader()
{
    return _pimpl->leader();
}

void Candidate::start()
{
    _pimpl->start();
}

void Candidate::handle(const event::NodeFailEvent &event)
{
    if (event.id == leader())
    {
        std::thread electThread(&Impl::electionThread, _pimpl.get());
        electThread.detach();
    }
}

int Candidate::requestPort()
{
    return _pimpl->requestPort;
}

void Candidate::setOkReceived()
{
    _pimpl->okReceived = true;
}

Candidate *Candidate::instance()
{
    static Candidate *candidate = nullptr;
    if (candidate == nullptr) candidate = new Candidate();
    return candidate;
}

void Candidate::replyOk()
{
    _pimpl->replyOk();
}

void Candidate::setNewLeader(const std::string &leaderId)
{
    _pimpl->setLeader(leaderId);
    auto previousLeader = _pimpl->previousLeaderId;
    if (leaderId != previousLeader)
    {
        _pimpl->previousLeaderId = leaderId;
        _pimpl->handleEvent(event::LeaderElectedEvent(leaderId));
        LOG(INFO) << "Election finished, new leader: " << _pimpl->leader() << std::endl;
    }
    _pimpl->setElectionFinished();
}

void Candidate::sendElectedConfirmation()
{
    _pimpl->sendElectionConfirmation();
}

void Candidate::startElection()
{
    std::thread electThread(&Impl::electionThread, _pimpl.get());
    electThread.detach();
}

void Candidate::addListener(int eventId, SimpleEventHandlerPointer handler)
{
    _pimpl->addListener(eventId, handler);
}

void Candidate::onElectionConfirmationReceive()
{
    _pimpl->onElectionConfirmationReceive();
}

bool Candidate::allConfirmationsReceived()
{
    return _pimpl->exptectedConfirmationCount == _pimpl->confirmationCount();
}
