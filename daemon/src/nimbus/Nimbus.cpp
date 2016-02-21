//
// Created by yura on 11/20/15.
//

#include "Nimbus.h"
#include <zmq.hpp>
#include <zmq.h>
#include <zmq_utils.h>
#include <Types.h>
#include <easylogging++.h>
#include <message/CraneMessageHandler.h>
#include <message/CraneMessageParser.h>
#include <SystemHelper.h>
#include <thread>

#include <crane.pb.h>
#include <topology/TopologySpec.h>
#include <src/filesys/DFile.h>
#include <src/DMember.h>
#include <vector>
#include <queue>
#include <src/supervisor/Supervisor.h>

struct Nimbus::Impl {
    Impl(Nimbus *parent)
            : parent(parent), context(1), serviceSocket(context, ZMQ_REP) {
        serviceSocket.bind(SystemHelper::makeAddr("*", servicePort).c_str());
        DMember::instance()->subscribe<event::NodeFailEvent>(parent);
    }

    void updateSupervisors()
    {
        supervisors.clear();
        auto tmp = DMember::instance()->membershipList();
        for (auto member : tmp)
        {
            if (!(member == DMember::instance()->id()))
            {
                supervisors.push_back(member);
            }
        }
    }

    std::string nextExecutor()
    {
        static std::string executor = "";
        if (executor.empty()) executor = supervisors[0];
        else {
            int pos = std::find(supervisors.begin(), supervisors.end(), executor)
                    - supervisors.begin();
            executor = supervisors[(++pos) % supervisors.size()];
        }
        return executor;
    }

    void startNimbusThread() {
        while (true) {
            try {
                zmq::message_t msg;
                bool success = serviceSocket.recv(&msg);
                if (success)
                    CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e) {
                LOG(ERROR) << e.what();
            }
        }
    }

    void start() {
        std::thread nimbus(&Impl::startNimbusThread, this);
        nimbus.detach();
    }

    void reply(const CraneWrappedMessagePointer &craneMsg) {
        zmq::message_t msg(craneMsg->ByteSize());
        craneMsg->SerializeToArray(msg.data(), craneMsg->ByteSize());

        serviceSocket.send(msg);
    }

    std::vector<std::string> getAllTopoIds()
    {
        std::vector<std::string> topoIds;
        for (auto it : topoStats)
        {
            topoIds.push_back(it.first);
        }
        return topoIds;
    }

    std::string getTopoStatus(const std::string &topoid)
    {
        if (topoStats.find(topoid) == topoStats.end()) return "nonexisted";
        else return topoStats[topoid].status;
    }

    void setTopoStatus(const std::string &topoid, const std::string &status)
    {
        if (topoStats.find(topoid) == topoStats.end()) return;
        topoStats[topoid].status = status;
    }

    std::vector<std::string> getTopoTaskInstances(const std::string &topoid, const std::string &name)
    {
        if (topoStats.find(topoid) == topoStats.end()) return std::vector<std::string>();
        if (topoStats[topoid].taskInstances.find(name) == topoStats[topoid].taskInstances.end())
            return std::vector<std::string>();
        return topoStats[topoid].taskInstances[name];
    }

    void setTopoTaskInstances(const std::string &topoid, const std::string &name, const std::vector<std::string> &instances)
    {
        if (topoStats.find(topoid) == topoStats.end()) return;
        topoStats[topoid].taskInstances[name] = instances;
    }

    void addTopoTaskInstance(const std::string &topoid, const std::string &name, const std::string &instance) {
        if (topoStats.find(topoid) == topoStats.end())
        {
            LOG(ERROR) << "Topoid: " << topoid << " does not exist";
            return;
        }
        topoStats[topoid].taskInstances[name].push_back(instance);
    }

    TopologySpec getTopoSpec(const std::string &topoid)
    {
        if (topoStats.find(topoid) == topoStats.end()) return TopologySpec();
        return topoStats[topoid].topoSpec;
    }

    void setTopoSpec(const std::string &topoid, const TopologySpec &spec)
    {
        topoStats[topoid].topoSpec = spec;
    }

    void compileTopology(const std::string &topoid, const std::vector<std::string> &sources)
    {
        LOG(DEBUG) << "compileTopology(" << topoid << ") called";
        // compile code
        std::string zipPath = "tmp/" + topoid + ".tar.gz";
        std::string extractPath = "tmp/code/";

        SystemHelper::exec("mkdir -p " + extractPath);
        SystemHelper::exec("rm -f " + extractPath + "*");
        std::string extractCommand = "tar zxf " + zipPath + " -C " + extractPath;
        SystemHelper::exec(extractCommand);
//g++ -std=c++11 -fPIC -shared EchoBolt.cpp -I . -I /home/e-al/Projects/C++/UIUC/CS425/cs425-mp4/crane-core/src -I ~/Projects/C++/UIUC/CS425/cs425-mp4/utils -o libtest.so
        for (auto source : sources)
        {
            std::string sourcePath = extractPath + source + ".cpp";
            std::string objectPath = extractPath + topoid + "_" + source + ".o";
            std::string libPath = extractPath + SystemHelper::getLibraryName(topoid, source);
            std::string compileCommand = "g++ " + sourcePath + " ";
            for (auto path : includePath)
            {
                compileCommand += "-I " + path + " ";
            }
            compileCommand += "-o " + libPath + " ";
            for (auto flag : cflags)
            {
                compileCommand += flag + " ";
            }
            LOG(DEBUG) << "compile library command: " << compileCommand;
            SystemHelper::exec(compileCommand);

            // put library on SDFS
            DFile::instance()->sendPutRequest(libPath);
        }

        setTopoStatus(topoid, "idle");
        LOG(DEBUG) << "compileTopology(" << topoid << ") finish";

    }

    void prepareTopology(const std::string &topoid)
    {
        LOG(DEBUG) << "prepareTopology(" << topoid << ") called";
        setTopoStatus(topoid, "preparing");

        LOG(DEBUG) << "start creating workers";
        updateSupervisors();
        std::map<std::string, std::priority_queue<std::string> > assignment;
        std::map<std::string, std::string> runMap;

        TopologySpec spec = getTopoSpec(topoid);
        for(auto ss : spec.spoutSpecs)
        {
            runMap[ss.name] = ss.runname;
            std::string executor = nextExecutor();
            assignment[executor].push(ss.name);
        }

        for(auto bs : spec.boltSpecs)
        {
            runMap[bs.name] = bs.runname;
            for(int i=0; i < bs.paraLevel; ++i)
            {
                std::string executor = nextExecutor();
                assignment[executor].push(bs.name);
            }
        }

        for (auto it : assignment)
        {
            // assign tasks
            zmq::socket_t socket(context, ZMQ_REQ);
            socket.connect(SystemHelper::makeAddr(it.first, Supervisor::instance()->port()).c_str());

            std::string supervisor = it.first;
            std::string name = "";
            int count;
            while (!assignment[supervisor].empty())
            {
                name = assignment[supervisor].top();
                count = 1;
                assignment[supervisor].pop();
                while (!assignment[supervisor].empty() && name == assignment[supervisor].top())
                {
                    ++count;
                    assignment[supervisor].pop();
                }

                CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
                craneWrappedMessage->set_type(crane::NIMBUS_REQ);
                craneWrappedMessage->mutable_cranenimbusrequest()->set_type(crane::CraneNimbusRequest_NimbusRequestType_ADD_WORKER);
                auto addReq = craneWrappedMessage->mutable_cranenimbusrequest()->mutable_addworkerrequest();
                addReq->set_topoid(topoid);
                addReq->set_name(name);
                addReq->set_runname(runMap[name]);
                addReq->set_instancenumber(count);


                zmq::message_t msg(craneWrappedMessage->ByteSize());
                craneWrappedMessage->SerializeToArray(msg.data(), craneWrappedMessage->ByteSize());

                try {
                    socket.send(msg);

                    msg.rebuild();
                    bool success = socket.recv(&msg);
                    if (success)
                        CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
                }
                catch (const std::exception &e)
                {
                    LOG(ERROR) << e.what();
                }
            }
            socket.close();
        }

        LOG(DEBUG) << "Done creating workers";

        LOG(DEBUG) << "Start setup connections";
        zmq_sleep(3);
        // setup connections
        std::map<std::string, std::vector<std::pair<std::string, std::string> > > tree;
        for (auto bs : spec.boltSpecs)
        {
            for (auto parent : bs.parents)
            {
                tree[parent.first].push_back(
                    std::make_pair(bs.name, parent.second)
                );
            }
        }


        CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
        craneWrappedMessage->set_type(crane::NIMBUS_REQ);
        craneWrappedMessage->mutable_cranenimbusrequest()->set_type(crane::CraneNimbusRequest_NimbusRequestType_CONFIG_CONNS);
        auto connReq = craneWrappedMessage->mutable_cranenimbusrequest()->mutable_configureconnectionsrequest();
        LOG(INFO) << "Send connection configs:";
        for (auto it : tree)
        {
            auto workerConfig = connReq->add_workerconnections();
            workerConfig->set_topoid(topoid);
            workerConfig->set_name(it.first);

            std::cout << "topoid: " << topoid << " name: " << it.first << std::endl;
            for (auto child : it.second)
            {
                std::cout << child.first << " " << child.second << std::endl;
                auto group = workerConfig->add_groupedaddresses();
                group->set_childname(child.first);
                group->set_grouping(child.second);
                for (auto address : getTopoTaskInstances(topoid, child.first))
                {
                    std::cout << address << " ";
                    group->add_addresses(address);
                }
                std::cout << std::endl;
            }
        }



        for (auto supervisor : supervisors)
        {
            zmq::socket_t socket(context, ZMQ_REQ);
            socket.connect(SystemHelper::makeAddr(supervisor, Supervisor::instance()->port()).c_str());

            zmq::message_t msg(craneWrappedMessage->ByteSize());
            craneWrappedMessage->SerializeToArray(msg.data(), craneWrappedMessage->ByteSize());

            try{
                socket.send(msg);

                msg.rebuild();
                bool success = socket.recv(&msg);
                if (success)
                    CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
            }
            catch (const std::exception &e)
            {
                LOG(ERROR) << e.what();
            }
        }
        setTopoStatus(topoid, "ready");

        LOG(DEBUG) << "Done setup connections";
        LOG(DEBUG) << "prepareTopology(" << topoid << ") finish";
    }

    void startTopology(const std::string &topoid)
    {
        LOG(DEBUG) << "startTopology(" << topoid << ") is called";

        if (getTopoStatus(topoid) != "ready")
        {
            LOG(ERROR) << topoid << " is not ready";
            return;
        }

        TopologySpec spec = getTopoSpec(topoid);

        std::vector<std::string> names;
        for (auto ss : spec.spoutSpecs)
        {
            names.push_back(ss.name);
        }

        for (auto bs : spec.boltSpecs)
        {
            names.push_back(bs.name);
        }

        updateSupervisors();
        for (auto supervisor : supervisors)
        {
            zmq::socket_t socket(context, ZMQ_REQ);
            socket.connect(SystemHelper::makeAddr(supervisor, Supervisor::instance()->port()).c_str());

            for (auto name : names)
            {
                CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
                craneWrappedMessage->set_type(crane::NIMBUS_REQ);
                craneWrappedMessage->mutable_cranenimbusrequest()->set_type(crane::CraneNimbusRequest_NimbusRequestType_START_WORKER);
                auto startReq = craneWrappedMessage->mutable_cranenimbusrequest()->mutable_startworkerrequest();
                startReq->set_topoid(topoid);
                startReq->set_name(name);

                zmq::message_t msg(craneWrappedMessage->ByteSize());
                craneWrappedMessage->SerializeToArray(msg.data(), craneWrappedMessage->ByteSize());

                try {
                    socket.send(msg);
                    msg.rebuild();
                    bool success = socket.recv(&msg);
                    if (success)
                        CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
                }
                catch (const std::exception &e)
                {
                    LOG(ERROR) << e.what();
                }
            }
            socket.close();

        }

        setTopoStatus(topoid, "running");
        LOG(DEBUG) << "startTopology(" << topoid << ") finish";
    }

    void stopTopology(const std::string &topoid)
    {
        LOG(DEBUG) << "stopTopology(" << topoid << ") is called";
        if (getTopoStatus(topoid) != "running")
        {
            LOG(WARNING) << topoid << " is not running";
        }

        TopologySpec spec = getTopoSpec(topoid);

        std::vector<std::string> names;
        for (auto ss : spec.spoutSpecs)
        {
            names.push_back(ss.name);
        }

        for (auto bs : spec.boltSpecs)
        {
            names.push_back(bs.name);
        }

        updateSupervisors();
        for (auto supervisor : supervisors)
        {
            zmq::socket_t socket(context, ZMQ_REQ);
            socket.connect(SystemHelper::makeAddr(supervisor, Supervisor::instance()->port()).c_str());

            for (auto name : names)
            {
                CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
                craneWrappedMessage->set_type(crane::NIMBUS_REQ);
                craneWrappedMessage->mutable_cranenimbusrequest()->set_type(crane::CraneNimbusRequest_NimbusRequestType_KILL_WORKER);
                auto startReq = craneWrappedMessage->mutable_cranenimbusrequest()->mutable_killworkerrequest();
                startReq->set_topoid(topoid);
                startReq->set_name(name);

                zmq::message_t msg(craneWrappedMessage->ByteSize());
                craneWrappedMessage->SerializeToArray(msg.data(), craneWrappedMessage->ByteSize());

                try {
                    socket.send(msg);
                    msg.rebuild();
                    bool success = socket.recv(&msg);
                    if (success)
                        CraneMessageParser::parse(msg.data(), msg.size(), &messageHandler);
                }
                catch (const std::exception &e)
                {
                    LOG(ERROR) << e.what();
                }
            }
            socket.close();
        }
        setTopoStatus(topoid, "idle");
        topoStats[topoid].taskInstances.clear();
        LOG(DEBUG) << "stopTopology(" << topoid << ") finish";
    }

    void killTopology(const std::string &topoid)
    {
        LOG(DEBUG) << "killTopology(" << topoid << ") is called";
        if (topoStats.find(topoid) == topoStats.end()) return;
        if (topoStats[topoid].status == "running") stopTopology(topoid);
        // TODO: delete lib file from SDFS


        topoStats.erase(topoid);
        LOG(DEBUG) << "killTopology(" << topoid << ") finish";
    }

    Nimbus *parent;
    zmq::context_t context;
    zmq::socket_t serviceSocket;

    const int servicePort = 4444;
    CraneMessageHandler messageHandler;

    struct TopologyTracker {
        std::string status;
        TopologySpec topoSpec;
        std::map<std::string, std::vector<std::string>> taskInstances;
    };

    std::map<std::string, TopologyTracker> topoStats;
    std::vector<std::string> supervisors;

    // compile path and flags
    const std::vector<std::string> includePath = {
        "~/src/cs425-mp4/crane-core/src",
        "~/src/cs425-mp4/utils"
    };

    const std::vector<std::string> cflags = {
        "-std=c++11",
        "-fPIC" ,
        "-shared"
    };

};

Nimbus::Nimbus()
    : _pImpl(new Impl(this))
{

}

Nimbus *Nimbus::instance()
{
    static Nimbus *nimbus = nullptr;
    if (nimbus == nullptr) nimbus = new Nimbus();
    return nimbus;
}

void Nimbus::start()
{
    _pImpl->start();
}

void Nimbus::reply(const CraneWrappedMessagePointer &msg) {
    _pImpl->reply(msg);
}

std::vector<std::string> Nimbus::getAllTopoIds() {
    return _pImpl->getAllTopoIds();
}

std::string Nimbus::getTopoStatus(const std::string &topoid) {
    return _pImpl->getTopoStatus(topoid);
}

void Nimbus::setTopoStatus(const std::string &topoid, const std::string &status) {
    _pImpl->setTopoStatus(topoid, status);
}

TopologySpec Nimbus::getTopoSpec(const std::string &topoid) {
    return _pImpl->getTopoSpec(topoid);
}

void Nimbus::setTopoSpec(const std::string &topoid, const TopologySpec &spec) {
    _pImpl->setTopoSpec(topoid, spec);
}

std::vector<std::string> Nimbus::getTopoTaskInstances(const std::string &topoid, const std::string &name) {
    return _pImpl->getTopoTaskInstances(topoid, name);
}

void Nimbus::setTopoTaskInstances(const std::string &topoid, const std::string &name, const std::vector<std::string> &instances) {
    _pImpl->setTopoTaskInstances(topoid, name, instances);
}

void Nimbus::addTopoTaskInstance(const std::string &topoid, const std::string &name, const std::string &instance) {
    _pImpl->addTopoTaskInstance(topoid, name, instance);
}

void Nimbus::stopTopology(const std::string &topoid) {
    _pImpl->stopTopology(topoid);
}

void Nimbus::killTopology(const std::string &topoid) {
    _pImpl->killTopology(topoid);
}

void Nimbus::startTopology(const std::string &topoid) {
    _pImpl->startTopology(topoid);
}

void Nimbus::prepareTopology(const std::string &topoid) {
    _pImpl->prepareTopology(topoid);
}

void Nimbus::compileTopology(const std::string &topoid, const std::vector<std::string> &sources) {
    _pImpl->compileTopology(topoid, sources);
}

void Nimbus::handle(const event::NodeFailEvent &event) {
    std::thread restartThread([this]()
        {
            auto topoIds = _pImpl->getAllTopoIds();
            for (auto topoId : topoIds)
            {
                _pImpl->stopTopology(topoId);
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
            for (auto topoId : topoIds)
            {
                _pImpl->prepareTopology(topoId);
                _pImpl->startTopology(topoId);
            }

        });

    restartThread.detach();
}

void Nimbus::handle(const event::LeaderElectedEvent &event) {

}
