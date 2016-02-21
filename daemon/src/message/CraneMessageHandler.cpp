//
// Created by yura on 11/20/15.
//

#include "CraneMessageHandler.h"
#include <crane.pb.h>
#include <topology/TopologySpec.h>
#include <src/nimbus/Nimbus.h>
#include <supervisor/Supervisor.h>
#include <fstream>
#include <SystemHelper.h>
#include <easylogging++.h>
#include <zmq_utils.h>
#include <zmq.h>

void CraneMessageHandler::process(const CraneClientKillRequestPointer &request) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_KILL);

    auto killReply = craneWrappedMessage->mutable_craneclientreply()->mutable_killreply();
    killReply->set_toponame(request->toponame());
    if(Nimbus::instance()->getTopoStatus(request->toponame()) != "nonexisted")
    {
        killReply->set_nonexist(false);
        Nimbus::instance()->reply(craneWrappedMessage);
        Nimbus::instance()->killTopology(request->toponame());
    }
    else {
        killReply->set_nonexist(true);
        Nimbus::instance()->reply(craneWrappedMessage);
    }
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneClientSubmitRequestPointer &request) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_SUBMIT);

    auto submitReply = craneWrappedMessage->mutable_craneclientreply()->mutable_submitreply();
    submitReply->set_toponame(request->toponame());
    TopologySpec topologySpec = TopologySpec::parseFromRequest(request);
    if (Nimbus::instance()->getTopoStatus(request->toponame()) != "nonexisted")
    {
        submitReply->set_approval(false);
    }
    else
    {
        submitReply->set_approval(true);
        Nimbus::instance()->setTopoSpec(request->toponame(), topologySpec);
        Nimbus::instance()->setTopoStatus(request->toponame(), "pending");
    }
    Nimbus::instance()->reply(craneWrappedMessage);
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneClientStartRequestPointer &request) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_START);

    auto startReply = craneWrappedMessage->mutable_craneclientreply()->mutable_startreply();
    startReply->set_toponame(request->toponame());
    if(Nimbus::instance()->getTopoStatus(request->toponame()) != "nonexisted")
    {
        startReply->set_nonexist(false);
        Nimbus::instance()->reply(craneWrappedMessage);
        Nimbus::instance()->prepareTopology(request->toponame());
        Nimbus::instance()->startTopology(request->toponame());
    }
    else {
        startReply->set_nonexist(true);
        Nimbus::instance()->reply(craneWrappedMessage);
    }
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneClientStopRequestPointer &request) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_STOP);

    auto stopReply = craneWrappedMessage->mutable_craneclientreply()->mutable_stopreply();
    stopReply->set_toponame(request->toponame());
    if(Nimbus::instance()->getTopoStatus(request->toponame()) != "nonexisted")
    {
        stopReply->set_nonexist(false);
        Nimbus::instance()->reply(craneWrappedMessage);
        Nimbus::instance()->stopTopology(request->toponame());
    }
    else {
        stopReply->set_nonexist(true);
        Nimbus::instance()->reply(craneWrappedMessage);
    }
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneClientSendRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    std::string filename = request->filename();
    std::string filepath = "tmp/" + filename;
    std::ofstream ofs(filepath, std::ios_base::out);
    ofs << request->data();
    ofs.flush();

    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_SEND);
    auto sendReply = craneWrappedMessage->mutable_craneclientreply()->mutable_sendreply();
    sendReply->set_toponame(request->toponame());


    if (false) {
        LOG(INFO) << SystemHelper::getmd5(filepath);
        LOG(INFO) << "MD5 does not match. Kill the topology: " << request->toponame();
        sendReply->set_md5match(false);
        // TODO: need to kill the topology
        Nimbus::instance()->reply(craneWrappedMessage);
        Nimbus::instance()->killTopology(request->toponame());
    }
    else {
        sendReply->set_md5match(true);
        Nimbus::instance()->reply(craneWrappedMessage);

        std::vector<std::string> sources;
        for (auto file : request->extractedfiles())
        {
            int pos = file.find(".cpp");
            if (pos != std::string::npos)
            {
                sources.push_back(file.substr(0, pos));
            }
        }
        Nimbus::instance()->compileTopology(request->toponame(), sources);
        zmq_sleep(2);
        Nimbus::instance()->prepareTopology(request->toponame());
        Nimbus::instance()->startTopology(request->toponame());
    }
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneClientStatusRequestPointer &request) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    CraneWrappedMessagePointer craneWrappedMessage = std::make_shared<crane::CraneWrappedMessage>();
    craneWrappedMessage->set_type(crane::CLIENT_REP);
    craneWrappedMessage->mutable_craneclientreply()->set_type(crane::CraneClientReply_ClientReplyType_STATUS);
    auto statusReply = craneWrappedMessage->mutable_craneclientreply()->mutable_statusreply();

    for (auto topoid : Nimbus::instance()->getAllTopoIds())
    {
        auto ts = statusReply->add_topostatus();
        ts->set_toponame(topoid);
        ts->set_status(Nimbus::instance()->getTopoStatus(topoid));

        TopologySpec spec = Nimbus::instance()->getTopoSpec(topoid);
        for (auto ss : spec.spoutSpecs)
        {
            for (auto address : Nimbus::instance()->getTopoTaskInstances(topoid, ss.name))
            {
                auto worker = ts->add_workers();
                worker->set_name(ss.name);
                worker->set_runname(ss.runname);
                worker->set_address(address);
            }
        }
        for (auto bs : spec.boltSpecs)
        {
            for (auto address : Nimbus::instance()->getTopoTaskInstances(topoid, bs.name))
            {
                auto worker = ts->add_workers();
                worker->set_name(bs.name);
                worker->set_runname(bs.runname);
                worker->set_address(address);
            }
        }

    }

    Nimbus::instance()->reply(craneWrappedMessage);
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}


// Nimbus process reply from supervisor
void CraneMessageHandler::process(const CraneNimbusAddWorkerReplyPointer &reply) {
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    if (reply->port_size() == 0)
    {
        LOG(ERROR) << "Add worker reply creates no worker";
        return;
    }
    for (auto port : reply->port())
    {
        // may need to adapt to protocol
        std::string instance = SystemHelper::makeAddr(reply->supervisorid(), port);
        Nimbus::instance()->addTopoTaskInstance(reply->topoid(), reply->name(), instance);
    }
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneNimbusConfigureConnectionsReplyPointer &/*reply*/) {

}

void CraneMessageHandler::process(const CraneNimbusAddWorkerRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    Supervisor::instance()->addWorker(request->topoid()
                                      , request->runname()
                                      , request->name()
                                      , request->instancenumber());
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneNimbusStartWorkerRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    Supervisor::instance()->startWorker(request->topoid(), request->name());
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneNimbusStartWorkerReplyPointer &reply)
{

}

void CraneMessageHandler::process(const CraneNimbusKillWorkerRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    Supervisor::instance()->killWorker(request->topoid(), request->name());
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void CraneMessageHandler::process(const CraneNimbusKillWorkerReplyPointer &reply)
{

}

void CraneMessageHandler::process(const CraneNimbusConfigureConnectionsRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    for (const auto& workerConnection : request->workerconnections())
    {
        std::list<Worker::ConnectionGroup> connectionGroups;
        for (const auto& groupAddresses : workerConnection.groupedaddresses())
        {
            connectionGroups.emplace_back();
            connectionGroups.back().childName = groupAddresses.childname();
            connectionGroups.back().grouping = groupAddresses.grouping();
            std::copy(groupAddresses.addresses().begin(), groupAddresses.addresses().end()
                      , std::back_inserter(connectionGroups.back().addresses));
        }

        Supervisor::instance()->configureConnections(workerConnection.topoid(), workerConnection.name()
                                                     , connectionGroups);

    }

    Supervisor::instance()->replyConfigureConnections();
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}
