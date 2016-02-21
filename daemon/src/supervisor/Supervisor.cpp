//
// Created by e-al on 27.11.15.
//

#include <easylogging++.h>
#include <zmq.hpp>

#include <mutex>
#include <queue>
#include <thread>
#include <cstring>

#include <filesys/DFile.h>
#include <DMember.h>
#include <tuple/Tuple.h>
#include <message/CraneMessageHandler.h>
#include <message/CraneMessageParser.h>
#include <SystemHelper.h>
#include <worker/Worker.h>
#include <crane.pb.h>
#include "Supervisor.h"


// map from WorkerId to list of workers (because we can run multiple instances if parallelism level is > 1)
using WorkersMap = std::map<std::string, std::list<WorkerPointer>>;
//using

struct Supervisor::Impl
{
    Impl(Supervisor *newParent)
    : parent(newParent)
    , context(1)
    , commandSocket(context, ZMQ_REP)
    , messageHandler()
    {

    }

    // Receive commands from Nimbus
    void commandRecvThread()
    {
        commandSocket.bind(SystemHelper::makeAddr("*", commandPort).c_str());
        zmq::message_t msg;

        while (true)
        {
            try
            {
//                LOG(DEBUG) << "Trying to receive message";
                commandSocket.recv(&msg);
                CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
//                LOG(DEBUG) << "Finished processing the message";
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
        }
    }

    void start()
    {
        std::thread commandReceivingThread(&Impl::commandRecvThread, this);
        commandReceivingThread.detach();
    }

    void addWorker(const std::string &topoId, const std::string &runName, const std::string& name
                   , int parallelismLevel)
    {
//        LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
        auto workerId = createWorkerId(topoId, name);
        auto insertPair = workersMap.insert(std::make_pair(workerId
                                                           , std::list<WorkerPointer>()));
        auto& workersList = insertPair.first->second;
        std::list<int> ports;
        std::string libname = SystemHelper::getLibraryName(topoId, runName);
        DFile::instance()->sendGetRequest(libname, libname);
        for (int i = 0; i < parallelismLevel; ++i)
        {
            int dataPort = nextAvailableWorkerPort();
            workersList.push_back(std::make_shared<Worker>(workerId, dataPort
                                                              , SystemHelper::getLibraryLocalPath(libname)
                                                              , SystemHelper::isBolt(runName)));
            ports.push_back(dataPort);
            std::string dataPortStr = std::to_string(dataPort);
            const char *workerProcessName = "./daemon";
            std::cout << "RUN NAME: " << runName << std::endl;
            std::string isBolt = SystemHelper::isBolt(runName) ? "-b" : "";
            std::cout << "BOLT PARAM: " << isBolt << std::endl;
            std::vector<std::string> args{workerProcessName, "-w", "-p", dataPortStr, "-a"
                    , workerId + dataPortStr, "-l", SystemHelper::getLibraryLocalPath(libname), isBolt};
//
            const char **argv = new const char *[args.size() + 1];
            for (int i = 0; i < args.size(); ++i)
            {
                argv[i] = args[i].c_str();
            }
            argv[args.size()] = NULL;
            int processId = fork();
            // if child process - execve() into worker process
            if (processId == 0)
            {
                std::cout << "Child process" << std::endl;
                execv("./daemon", (char **)argv);

            }
            else if (processId > 0)
            {
                std::cout << "Parent process" << std::endl;
            }
            else
            {
                std::cout << "Fork failed" << std::endl;
            }
        }

        replyAddWorker(topoId, name, ports);
//        LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
    }

    void replyAddWorker(const std::string& topoId, const std::string& name, const std::list<int>& ports)
    {
//        LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
        auto workerReplyMsg = createAddWorkerReplyMessage(topoId, name, ports);
        replyToNimbus(workerReplyMsg);
//        LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
    }

    CraneWrappedMessagePointer createAddWorkerReplyMessage(const std::string& topoId
                                                           , const std::string name
                                                           , const std::list<int>& ports)
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::CraneType::NIMBUS_REP);

        msg->mutable_cranenimbusreply()->set_type(crane::CraneNimbusReply::ADD_WORKER);
        auto addWorkerReplyMsg = msg->mutable_cranenimbusreply()->mutable_addworkerreply();
        addWorkerReplyMsg->set_topoid(topoId);
        addWorkerReplyMsg->set_name(name);
        addWorkerReplyMsg->set_supervisorid(DMember::instance()->id());
        for (const auto& port : ports)
        {
            addWorkerReplyMsg->add_port(port);
        }

        return msg;
    }

    CraneWrappedMessagePointer createStartReplyMessage()
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::CraneType::NIMBUS_REP);
        msg->mutable_cranenimbusreply()->set_type(crane::CraneNimbusReply::START_WORKER);

        return msg;
    }

    CraneWrappedMessagePointer createKillReplyMessage()
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::CraneType::NIMBUS_REP);
        msg->mutable_cranenimbusreply()->set_type(crane::CraneNimbusReply::KILL_WORKER);

        return msg;
    }

    CraneWrappedMessagePointer createConfigureConnectionsReplyMessage()
    {
        CraneWrappedMessagePointer msg = std::make_shared<crane::CraneWrappedMessage>();
        msg->set_type(crane::CraneType::NIMBUS_REP);
        msg->mutable_cranenimbusreply()->set_type(crane::CraneNimbusReply::CONFIG_CONNS);

        return msg;
    }

    void configureConnections(const std::string &topoId, const std::string &name
                              , const std::list<Worker::ConnectionGroup> &connectionGroups)
    {
        auto workerId = createWorkerId(topoId, name);
        auto workerIt = workersMap.find(workerId);

        if (workerIt != workersMap.end())
        {
            for (auto& worker : workerIt->second)
            {
//                std::cout << "Try to configure conns on worker " << workerId << std::endl;
//                worker->test();
                worker->configureConnections(connectionGroups);
//                std::cout << "Configured conns on worker " << workerId << std::endl;
            }
        }
    }

    void startWorker(const std::string& topoId, const std::string& name)
    {
        auto workerId = createWorkerId(topoId, name);
        auto workerIt = workersMap.find(workerId);
        std::cout << "Trying to start workers with id " << workerId << std::endl;

        if (workerIt != workersMap.end())
        {
            for (auto& worker : workerIt->second)
            {
                worker->start();
            }
        }

        std::cout << "Trying to reply from startWorker()" << std::endl;
        replyToNimbus(createStartReplyMessage());
    }

    void killWorker(const std::string& topoId, const std::string& name)
    {
        auto workerId = createWorkerId(topoId, name);
        auto workerIt = workersMap.find(workerId);

        if (workerIt != workersMap.end())
        {
            for (auto& worker : workerIt->second) { worker->kill(); }
            workersMap.erase(workerId);
        }

        replyToNimbus(createKillReplyMessage());
    }

    int nextAvailableWorkerPort()
    {
        static int port = workerBasePort;
        return port++;
    }

    void replyToNimbus(const CraneWrappedMessagePointer& message)
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


    void replyConfigureConnections()
    {
        replyToNimbus(createConfigureConnectionsReplyMessage());
    }

    // ID represented as ipc address
    std::string createWorkerId(const std::string& topoId, const std::string& name)
    {
        std::string workerId{"ipc:///tmp/"};
        workerId.append(topoId);
//        workerId.append("");
        workerId.append(name);

        return workerId;
    }


    Supervisor *parent;
    zmq::context_t context;
    zmq::socket_t commandSocket;
    CraneMessageHandler messageHandler;
    WorkersMap workersMap;


    const int commandPort = 5432;
    const int workerBasePort = 3000;
};

Supervisor *Supervisor::instance()
{
    static Supervisor *supervisor = nullptr;
    if (supervisor == nullptr) supervisor = new Supervisor();

    return supervisor;
}

void Supervisor::start()
{
    _pimpl->start();
}

Supervisor::Supervisor()
    : _pimpl(new Impl(this))
{

}

void Supervisor::addWorker(const std::string &topoId, const std::string &runName
                           , const std::string &name, int parallelismLevel)
{
    _pimpl->addWorker(topoId, runName, name, parallelismLevel);
}

int Supervisor::port()
{
    return _pimpl->commandPort;
}

void Supervisor::startWorker(const std::string &topoId, const std::string &name)
{
    _pimpl->startWorker(topoId, name);
}

void Supervisor::killWorker(const std::string &topoId, const std::string &name)
{
    _pimpl->killWorker(topoId, name);
}

void Supervisor::configureConnections(const std::string &topoId, const std::string &name
                                      , const std::list<Worker::ConnectionGroup> &connectionGroups)
{
    _pimpl->configureConnections(topoId, name, connectionGroups);
}

void Supervisor::replyConfigureConnections()
{
    _pimpl->replyConfigureConnections();
}
