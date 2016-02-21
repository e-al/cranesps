//
// Created by e-al on 07.11.15.
//

#ifndef DMEMBER_ELECTIONMESSAGEHANDLER_H
#define DMEMBER_ELECTIONMESSAGEHANDLER_H


#include <zmq.hpp>

#include <election.pb.h>

#include <Types.h>

class ElectionMessageHandler
{
public:
    ElectionMessageHandler() = default;
    virtual ~ElectionMessageHandler() = default;


    void process(const ElectionStartPointer &request);
    void process(const ElectionOkMessagePointer &okMessage);
    void process(const ElectionNewLeaderPointer& newLeader);
    void process(const ElectionConfirmPointer &confirm);
};


#endif //DMEMBER_ELECTIONMESSAGEHANDLER_H
