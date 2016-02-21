//
// Created by e-al on 28.08.15.
//

#ifndef DGREP_MESSAGEHANDLER_H
#define DGREP_MESSAGEHANDLER_H

#include <zmq.hpp>

#include <dmember.pb.h>

#include <Types.h>

class MessageHandler
{
public:
    MessageHandler() = default;
    virtual ~MessageHandler() = default;


    void process(const PingAckPointer &ack);
    void process(const JoinRequestPointer &request);
    void process(const JoinAckPointer &ack);
    void process(const PingRequestPointer &request);
    void process(const HeartBeatPointer& heartbeat);
};


#endif //DGREP_MESSAGEHANDLER_H
