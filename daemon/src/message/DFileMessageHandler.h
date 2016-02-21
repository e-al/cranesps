//
// Created by e-al on 28.08.15.
//

#ifndef DGREP_MESSAGEHANDLER_H
#define DGREP_MESSAGEHANDLER_H

#include <zmq.hpp>


#include <Types.h>

class DFileMessageHandler
{
public:
    DFileMessageHandler() = default;
    virtual ~DFileMessageHandler() = default;


    void process(const SendRequestPointer &request);
    void process(const SendReplyPointer &reply);
    void process(const FetchRequestPointer &reqest);
    void process(const FetchReplyPointer &reply);
    void process(const ServiceRequestPointer &request);
    void process(const ServiceReplyPointer &reply);
    void process(const CommandRequestPointer &request);
    void process(const CommandReplyPointer &reply);
};


#endif //DGREP_MESSAGEHANDLER_H
