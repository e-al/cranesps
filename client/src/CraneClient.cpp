//
// Created by yura on 11/19/15.
//

#include "CraneClient.h"
#include <zmq.hpp>
#include <iostream>
#include <SystemHelper.h>
#include <Types.h>
#include <crane.pb.h>

#include <message/MessageHandler.h>
#include <message/MessageParser.h>
#include <easylogging++.h>
#include <FileHelper.h>

struct Client::Impl {
    Impl(Client *parent)
        : parent(parent)
        , context(1)
        , nimbus("")
        , port(0)
    {

    }

    int nextSendPort()
    {
        static int port = sendPortBase;
        if (port == sendPortMax) port = sendPortBase;
        return port++;
    }

    void config(const std::string &nimbus, int port)
    {
        this->nimbus = nimbus;
        this->port = port;
    }

    void sendMsg(const CraneWrappedMessagePointer &craneWrappedMessage)
    {
        zmq::message_t msg(craneWrappedMessage->ByteSize());
        craneWrappedMessage->SerializeToArray(msg.data(), craneWrappedMessage->ByteSize());

        try {
            zmq::socket_t socket(context, ZMQ_REQ);
            socket.connect(SystemHelper::makeAddr(nimbus, port).c_str());

            socket.send(msg);
            msg.rebuild();
            bool success = socket.recv(&msg);
            if (success)
                MessageParser::parse(msg.data(), msg.size(), &messageHandler);
        }
        catch (const std::exception &e)
        {
            LOG(ERROR) << e.what();
        }
    }

    void submit(TopologySpec spec, const std::string &dir) {

        path[spec.name] = dir;

        std::vector<std::string> files;
        if (SystemHelper::getdir(dir, files) != 0)
        {
            LOG(ERROR) << "directory: " << dir << " does not exist. Submission aborted.";
            return;
        }

        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_SUBMIT);
        auto submitRequest = craneWrappedMessage->mutable_craneclientrequest()->mutable_submitrequest();
        submitRequest->set_toponame(spec.name);

        // set boltspecs
        for (BoltSpec bs : spec.boltSpecs)
        {
            auto boltSpec = submitRequest->add_boltspecs();
            boltSpec->set_name(bs.name);
            for (std::pair<std::string, std::string> pair : bs.parents)
            {
                auto parent = boltSpec->add_parents();
                parent->set_name(pair.first);
                parent->set_grouping(pair.second);
            }
            boltSpec->set_runname(bs.runname);
            if (bs.paraLevel > 3) boltSpec->set_paralevel(bs.paraLevel);
        }

        // set spoutspecs
        for (SpoutSpec ss : spec.spoutSpecs)
        {
            auto spoutSpec = submitRequest->add_spoutspecs();
            spoutSpec->set_name(ss.name);
            spoutSpec->set_runname(ss.runname);
        }

        // set ackerbolt
        submitRequest->set_ackerbolt(spec.ackerbolt);

        sendMsg(craneWrappedMessage);

    }

    void start(const std::string &topoid)
    {
        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_START);
        auto startRequest = craneWrappedMessage->mutable_craneclientrequest()->mutable_startrequest();
        startRequest->set_toponame(topoid);

        sendMsg(craneWrappedMessage);
    }

    void stop(const std::string &topoid)
    {
        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_STOP);
        auto stopRequest = craneWrappedMessage->mutable_craneclientrequest()->mutable_stoprequest();
        stopRequest->set_toponame(topoid);

        sendMsg(craneWrappedMessage);
    }

    void kill(const std::string &topoid)
    {
        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_KILL);
        auto killRequest = craneWrappedMessage->mutable_craneclientrequest()->mutable_killrequest();
        killRequest->set_toponame(topoid);

        sendMsg(craneWrappedMessage);
    }

    void status()
    {
        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_STATUS);

        sendMsg(craneWrappedMessage);

    }

    void send(const std::string &topoid)
    {
        if (path.find(topoid) == path.end())
        {
            LOG(ERROR) << "path of topology: " << topoid << " not in cache";
            return;
        }
        std::string dir = path[topoid];
        std::vector<std::string> files;

        if (SystemHelper::getdir(dir, files) != 0)
        {
            LOG(ERROR) << "directory: " << dir << " does not exist. Sending aborted.";
            return;
        }

        std::string compress = "tar czf /tmp/" + topoid + ".tar.gz " + "-C " + dir + " .";
        SystemHelper::exec(compress);

        std::string filepath = "/tmp/" + topoid + ".tar.gz";


        int sendPort;
        while(true)
        {
            try{
                sendPort = nextSendPort();
                zmq::socket_t testSocket(context, ZMQ_REP);
                testSocket.bind(SystemHelper::makeAddr("*", sendPort).c_str());
                testSocket.close();
                break;
            }
            catch (const std::exception &e)
            {
                LOG(WARNING) << "port " << sendPort << " in use, try next";
            }
        }

        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::CLIENT_REQ);
        craneWrappedMessage->mutable_craneclientrequest()->set_type(crane::CraneClientRequest_ClientRequestType_SEND);
        auto sendRequest = craneWrappedMessage->mutable_craneclientrequest()->mutable_sendrequest();

        std::string filename = basename(filepath.c_str());
        FileHelper fh(filepath, 1024*1024);
        int filesize = fh.getSize();
        std::string filemd5 = SystemHelper::getmd5(filepath);
        sendRequest->set_toponame(topoid);
        sendRequest->set_filename(filename);
        sendRequest->set_port(sendPort);
        sendRequest->set_filesize(filesize);
        sendRequest->set_filemd5(filemd5);

        sendRequest->set_data(fh.readAll());
        for (auto file : files)
        {
            sendRequest->add_extractedfiles(file);
        }

        sendMsg(craneWrappedMessage);
    }

    Client *parent;
    zmq::context_t context;


    std::string nimbus;
    int port;
    const int sendPortBase = 11100;
    const int sendPortMax = 11200;

    MessageHandler messageHandler;

    std::map<std::string, std::string> path;
};

Client::Client()
    : _pImpl(new Impl(this))
{

}

void Client::submit(TopologySpec spec, const std::string &dir) {
    _pImpl->submit(spec, dir);
}

void Client::config(const std::string &ip, int port) {
    _pImpl->config(ip, port);
}

void Client::send(const std::string &topoid) {
    _pImpl->send(topoid);
}

Client *Client::instance() {
    static Client *client = nullptr;
    if (client == nullptr) client = new Client();
    return client;
}

void Client::start(const std::string &topoid) {
    _pImpl->start(topoid);
}

void Client::stop(const std::string &topoid) {
    _pImpl->stop(topoid);
}

void Client::kill(const std::string &topoid) {
    _pImpl->kill(topoid);
}

void Client::status() {
    _pImpl->status();
}
