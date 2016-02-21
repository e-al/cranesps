//
// Created by yura on 11/20/15.
//

#ifndef DMEMBER_NIMBUSMESSAGEHANDLER_H
#define DMEMBER_NIMBUSMESSAGEHANDLER_H


#include <Types.h>

class CraneMessageHandler {

public:
    CraneMessageHandler() = default;
    virtual ~CraneMessageHandler() = default;

//    virtual void process(const CraneConfigureConnectionsPointer& connections);

    // Methods that should be reimplemented by specific handler subclass
    virtual void process(const CraneNimbusAddWorkerRequestPointer &request);
    virtual void process(const CraneNimbusKillWorkerRequestPointer &request);
    virtual void process(const CraneNimbusStartWorkerRequestPointer &request);
    virtual void process(const CraneNimbusStartWorkerReplyPointer &reply);
    virtual void process(const CraneNimbusKillWorkerReplyPointer &reply);
    virtual void process(const CraneTuplePointer &tuple) {}
    virtual void process(const CraneTupleBatchPointer &batch) {}
    virtual void process(const CraneNimbusConfigureConnectionsRequestPointer& request);
    virtual void process(const CraneSupervisorConfigureConnectionsRequestPointer &connections) {}
    virtual void process(const CraneSupervisorConfigureConnectionsReplyPointer &connections) {}
    virtual void process(const CraneSupervisorStartRequestPointer &connections) {}
    virtual void process(const CraneSupervisorStartReplyPointer &connections) {}
    virtual void process(const CraneSupervisorKillRequestPointer &connections) {}
    virtual void process(const CraneSupervisorKillReplyPointer &connections) {}

    // Nimbus process request from client
    void process(const CraneClientSubmitRequestPointer &request);
    void process(const CraneClientStartRequestPointer &request);
    void process(const CraneClientStopRequestPointer &request);
    void process(const CraneClientKillRequestPointer &request);
    void process(const CraneClientSendRequestPointer &request);
    void process(const CraneClientStatusRequestPointer &request);

    // Nimbus process reply from Supervisor
    void process(const CraneNimbusAddWorkerReplyPointer &reply);
    void process(const CraneNimbusConfigureConnectionsReplyPointer& reply);
};


#endif //DMEMBER_NIMBUSMESSAGEHANDLER_H
