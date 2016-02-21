//
// Created by e-al on 28.11.15.
//

#ifndef DMEMBER_WORKERMESSAGEHANDLER_H
#define DMEMBER_WORKERMESSAGEHANDLER_H

#include "CraneMessageHandler.h"
#include <worker/Worker.h>

//class Worker;

class WorkerMessageHandler : public CraneMessageHandler
{
public:
    WorkerMessageHandler(Worker* newWorker) : worker(newWorker) {}
    virtual ~WorkerMessageHandler() = default;

    virtual void process(const CraneTuplePointer &tuple) override;
    virtual void process(const CraneTupleBatchPointer &batch) override;
    virtual void process(const CraneSupervisorConfigureConnectionsRequestPointer &connections) override;
    virtual void process(const CraneSupervisorStartRequestPointer &request) override;
    virtual void process(const CraneSupervisorKillRequestPointer &request) override;

private:
    Worker* worker;
};


#endif //DMEMBER_WORKERMESSAGEHANDLER_H
