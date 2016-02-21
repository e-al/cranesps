//
// Created by e-al on 29.08.15.
//

#include <iostream>
#include <exception>

#include <easylogging++.h>
#include <dfile.pb.h>

#include "DFileMessageHandler.h"
#include "DFileMessageParser.h"

void DFileMessageParser::parse(BufferPointer buffer, SimpleDFileMessageHandlerPointer handler)
{ parse(buffer.get(), handler); }

void DFileMessageParser::parse(SimpleBufferPointer buffer, SimpleDFileMessageHandlerPointer handler)
{ parse(buffer->data(), buffer->size(), handler); }

void DFileMessageParser::parse(const void* buffer, int size, SimpleDFileMessageHandlerPointer handler)
{
    dfile::WrappedDFileMessage msg;
    if (!msg.ParseFromArray(buffer, size))
    {
        throw std::logic_error("Cannot parse the message");
    }

    switch (msg.type())
    {
        case dfile::SERVICE_REQ:
        {
            ServiceRequestPointer serviceReq(msg.release_servicerequest());
            handler->process(serviceReq);
            break;
        }

        case dfile::SERVICE_REP:
        {
            ServiceReplyPointer serviceRep(msg.release_servicereply());
            handler->process(serviceRep);
            break;
        }

        case dfile::SEND_REQ:
        {
            SendRequestPointer sendReq(msg.release_sendrequest());
            handler->process(sendReq);
            break;
        }

        case dfile::SEND_REP:
        {
            SendReplyPointer sendRep(msg.release_sendreply());
            handler->process(sendRep);
            break;
        }


        case dfile::FETCH_REQ:
        {
            FetchRequestPointer fetchReq(msg.release_fetchrequest());
            handler->process(fetchReq);
            break;
        }

        case dfile::FETCH_REP:
        {
            FetchReplyPointer fetchRep(msg.release_fetchreply());
            handler->process(fetchRep);
            break;
        }

        case dfile::COMMAND_REQ:
        {
            CommandRequestPointer commandReq(msg.release_commandrequest());

            handler->process(commandReq);
            break;
        }

        case dfile::COMMAND_REP:
        {
            CommandReplyPointer commandRep(msg.release_commandreply());
            handler->process(commandRep);
            break;
        }

        default:
        {
            LOG(ERROR) << "Message type is not recongnized";
        }
    }
}
