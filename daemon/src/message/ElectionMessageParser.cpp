//
// Created by e-al on 07.11.15.
//

#include "ElectionMessageParser.h"
#include <iostream>
#include <exception>

#include <election.pb.h>

#include "ElectionMessageHandler.h"

void ElectionMessageParser::parse(BufferPointer buffer, SimpleElectionMessageHandlerPointer handler)
{ parse(buffer.get(), handler); }

void ElectionMessageParser::parse(SimpleBufferPointer buffer, SimpleElectionMessageHandlerPointer handler)
{ parse(buffer->data(), buffer->size(), handler); }

void ElectionMessageParser::parse(const void* buffer, int size, SimpleElectionMessageHandlerPointer handler)
{
    election::ElectionWrappedMessage msg;
    if (!msg.ParseFromArray(buffer, size))
    {
        throw std::logic_error("Cannot parse the message");
    }

    switch (msg.type())
    {
        case election::ELECTION_START:
        {
            ElectionStartPointer electionStart(msg.release_electionstart());
            handler->process(electionStart);
            break;
        }

        case election::OK_MSG:
        {
            ElectionOkMessagePointer okMessage(msg.release_okmessage());
            handler->process(okMessage);
            break;
        }

        case election::NEW_LEADER:
        {
            ElectionNewLeaderPointer newLeader(msg.release_newleader());
            handler->process(newLeader);
            break;
        }

        case election::ELECT_CONFIRM:
        {
            ElectionConfirmPointer confirm(msg.release_electionconfirm());
            handler->process(confirm);
            break;
        }

        default:
        {
            std::cerr << "Message type is not recongnized" << std::endl;
        }
    }
}
