//
// Created by Tianyuan Liu on 9/23/15.
//

#include "Introducer.h"
#include <DMember.h>

#include <zmq.hpp>
#include <easylogging++.h>
#include <thread>
#include <SystemHelper.h>
#include <message/MessageParser.h>
#include <message/MessageHandler.h>
#include <member/Member.h>
#include <mutex>

struct Introducer::Impl {
    Impl(Introducer *newparent, DMember *newDmember)
        : parent(newparent)
        , dmember(newDmember)
        , context(1)
        , serviceSocket(context, ZMQ_REP)
        , memberMap(std::make_shared<MemberMap>())
    {
        addMembers();
        std::string serviceAddr = SystemHelper::makeAddr("*", servicePort());
        serviceSocket.bind(serviceAddr.c_str());


    }

    void addMembers()
    {
        const std::string IPPrefix = "fa15-cs425-g10-0";
        const std::string IPSuffix = ".cs.illinois.edu";
        std::stringstream ss;
        for (int id=1; id<=7; ++id)
        {
            ss.str("");
            ss << IPPrefix << std::to_string(id) << IPSuffix;
            potentialMembers.push_back(ss.str());
        }
    }

    void checkAvailabilityThread(const std::string &member)
    {
        const int timeout = 1000;
        zmq::socket_t socket(context, ZMQ_REQ);
        socket.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        std::string memberAddr = SystemHelper::makeAddr(member, dmember->pingPort());
        socket.connect(memberAddr.c_str());

        WrappedMessagePointer wrappedMessage = std::make_shared<dmember::WrappedMessage>();
        wrappedMessage->set_type(dmember::PING_REQ);

        zmq::message_t msg(wrappedMessage->ByteSize());
        wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());
        bool success = false;
        try {
            socket.send(msg);
            msg.rebuild();
            success = socket.recv(&msg);
            if (success) {
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }

        socket.close();
    }

    void checkAvailability()
    {
        activeMembers.clear();
        memberMap->clear();
        std::vector<std::thread> checkThreads;
        for (const auto& member : potentialMembers)
        {
           //TODO: complete send/recv functionality
            checkThreads.push_back(std::thread(&Impl::checkAvailabilityThread, this, member));
        }

        for (auto& thread : checkThreads)
        {
            thread.join();
        }

        dmember->updateConnectionsOnNextHeartbeat(memberMap);
    }


    void serviceThread()
    {
        while (true)
        {
            try {

                zmq::message_t msg;
                serviceSocket.recv(&msg);
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (...) {
                LOG(ERROR) << "Bad introducer service request";
            }
        }
    }

    void start()
    {
        std::thread thread(&Impl::serviceThread, this);
        thread.detach();
        LOG(INFO) << "Introducer start";
    }

    void replyJoinRequest(const std::string &member)
    {
        WrappedMessagePointer wrappedMessage = std::make_shared<dmember::WrappedMessage>();
        wrappedMessage->set_type(dmember::JOIN_ACK);
        wrappedMessage->mutable_joinack()->set_jointime(memberJoinTime[member].time_since_epoch().count());

        zmq::message_t msg(wrappedMessage->ByteSize());
        wrappedMessage->SerializeToArray(msg.data(), wrappedMessage->ByteSize());
        serviceSocket.send(msg);

    }


    Introducer *parent;
    DMember *dmember;
    zmq::context_t context;
//    zmq::socket_t updateSocket;
    zmq::socket_t serviceSocket;
    MessageHandler messageHandler;
    std::vector<std::string> potentialMembers;
    std::vector<std::string> activeMembers;
    std::map<std::string, TimeStamp> memberJoinTime;
    MemberMapPointer memberMap;

    const int memberPort = 9876; // check member availability & send member update
    std::mutex mutex;

};

Introducer::Introducer(DMember *newparent)
    : parent(newparent)
    , _pImpl(new Impl(this, parent))
{

}

// add active member and update its join time (in case of introducer rejoin)

void Introducer::recordAndReply(const std::string &member)
{
    _pImpl->memberJoinTime[member] = std::chrono::high_resolution_clock::now();
    _pImpl->replyJoinRequest(member);
}

void Introducer::start()
{
    _pImpl->start();
    LOG(INFO) << "Start as introducer";
}

void Introducer::updateNodes()
{
    _pImpl->checkAvailability();
}

int Introducer::servicePort()
{
    return 9877;
}

void Introducer::updateMember(const MemberPointer& member)
{
    std::unique_lock<std::mutex> lock(_pImpl->mutex);
    _pImpl->memberMap->insert({ member->id(), member });
}

