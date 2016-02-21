//
// Created by e-al on 28.08.15.
//

#ifndef CRANE_CLIENT_MESSAGEHANDLER_H
#define CRANE_CLIENT_MESSAGEHANDLER_H

#include <Types.h>

class MessageHandler
{
public:
    MessageHandler() = default;
    virtual ~MessageHandler() = default;

    void process(const CraneClientSubmitReplyPointer &reply);
    void process(const CraneClientStartReplyPointer &reply);
    void process(const CraneClientStopReplyPointer &reply);
    void process(const CraneClientKillReplyPointer &reply);
    void process(const CraneClientSendReplyPointer &reply);
    void process(const CraneClientStatusReplyPointer &reply);

};


#endif //CRANE_CLIENT_MESSAGEHANDLER_H
