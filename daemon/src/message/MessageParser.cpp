//
// Created by e-al on 29.08.15.
//

#include <iostream>
#include <exception>

#include <dmember.pb.h>

#include "MessageHandler.h"
#include "MessageParser.h"

void MessageParser::parse(BufferPointer buffer, SimpleMessageHandlerPointer handler)
{ parse(buffer.get(), handler); }

void MessageParser::parse(SimpleBufferPointer buffer, SimpleMessageHandlerPointer handler)
{ parse(buffer->data(), buffer->size(), handler); }

void MessageParser::parse(const void* buffer, int size, SimpleMessageHandlerPointer handler)
{
    dmember::WrappedMessage msg;
    if (!msg.ParseFromArray(buffer, size))
    {
        throw std::logic_error("Cannot parse the message");
    }

    switch (msg.type())
    {
        // received by introducer
        case dmember::PING_ACK:
        {
            PingAckPointer pingAck(msg.release_pingack());
            handler->process(pingAck);
            break;
        }

        case dmember::JOIN_REQ:
        {
            JoinRequestPointer joinReq(msg.release_joinrequest());
            handler->process(joinReq);
            break;
        }

        // received by a member
        case dmember::HEARTBEAT:
        {
            HeartBeatPointer heartbeat(msg.release_heartbeat());
            handler->process(heartbeat);
            break;
        }

        case dmember::JOIN_ACK:
        {
            JoinAckPointer joinAck(msg.release_joinack());
            handler->process(joinAck);
            break;
        }

        case dmember::PING_REQ:
        {
            PingRequestPointer pingReq(msg.release_pingrequest());
            handler->process(pingReq);
            break;
        }

        default:
        {
            std::cerr << "Message type is not recongnized" << std::endl;
        }
    }
}
