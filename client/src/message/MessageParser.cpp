//
// Created by e-al on 29.08.15.
//

#include <iostream>
#include <exception>

#include <crane.pb.h>

#include "MessageHandler.h"
#include "MessageParser.h"

void MessageParser::parse(BufferPointer buffer, SimpleMessageHandlerPointer handler)
{ parse(buffer.get(), handler); }

void MessageParser::parse(SimpleBufferPointer buffer, SimpleMessageHandlerPointer handler)
{ parse(buffer->data(), buffer->size(), handler); }

void MessageParser::parse(const void* buffer, int size, SimpleMessageHandlerPointer handler)
{
    crane::CraneWrappedMessage msg;
    if (!msg.ParseFromArray(buffer, size))
    {
        throw std::logic_error("Cannot parse the message");
    }

    switch (msg.type())
    {
        // do nothing receiving ack
        case crane::CLIENT_REP:
        {
            CraneClientReplyPointer clientRep(msg.release_craneclientreply());
            switch (clientRep->type())
            {
                case crane::CraneClientReply_ClientReplyType_SUBMIT:
                {
                    CraneClientSubmitReplyPointer submitRep(clientRep->release_submitreply());
                    handler->process(submitRep);
                    break;
                }
                case crane::CraneClientReply_ClientReplyType_SEND:
                {
                    CraneClientSendReplyPointer sendRep(clientRep->release_sendreply());
                    handler->process(sendRep);
                    break;
                }
                case crane::CraneClientReply_ClientReplyType_START:
                {
                    CraneClientStartReplyPointer startRep(clientRep->release_startreply());
                    handler->process(startRep);
                    break;
                }
                case crane::CraneClientReply_ClientReplyType_STOP:
                {
                    CraneClientStopReplyPointer stopRep(clientRep->release_stopreply());
                    handler->process(stopRep);
                    break;
                }
                case crane::CraneClientReply_ClientReplyType_KILL:
                {
                    CraneClientKillReplyPointer killRep(clientRep->release_killreply());
                    handler->process(killRep);
                    break;
                }
                case crane::CraneClientReply_ClientReplyType_STATUS:
                {
                    CraneClientStatusReplyPointer statusRep(clientRep->release_statusreply());
                    handler->process(statusRep);
                    break;
                }
            }
        }
            break;

        default:
        {
            std::cerr << "Message type is not recongnized" << std::endl;
        }
    }
}
