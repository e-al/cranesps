//
// Created by e-al on 28.08.15.
//

#include <iostream>
#include <list>
#include <sstream>
#include <src/DMember.h>
#include <member/Member.h>
#include "MessageHandler.h"


void MessageHandler::process(const PingAckPointer &ack)
{
    MemberPointer member = std::make_shared<Member>(ack->id(), 0, ack->count()
                                                    , std::chrono::high_resolution_clock::now(), false);
    DMember::instance()->introducer->updateMember(member);
}

void MessageHandler::process(const JoinRequestPointer &request)
{
    DMember::instance()->introducer->recordAndReply(request->id());
    DMember::instance()->introducer->updateNodes();
}

void MessageHandler::process(const JoinAckPointer &ack)
{
    // update local join time
    DMember::instance()->updateJoinTime(ack->jointime());
}

void MessageHandler::process(const PingRequestPointer& /*request*/)
{
    DMember::instance()->replyPingRequest();
}

void MessageHandler::process(const HeartBeatPointer &heartbeat)
{
    MemberMapPointer members = std::make_shared<MemberMap>();
    std::list<MemberId> leavingNodes;
    for (const auto& member : heartbeat->memberlist())
    {
        TimeStamp ts = TimeStamp() + std::chrono::nanoseconds(member.timestamp());
        auto memberPtr = std::make_shared<Member>(member.id(), 0, member.count(), ts, false, member.leaving());

        if (memberPtr->isLeaving())
        {
            leavingNodes.push_back(member.id());
        }

        members->insert({ memberPtr->id(), memberPtr });
    }

    bool needToUpdate = heartbeat->updateconnection();
    for (const auto& node : leavingNodes)
    {
        if (!DMember::instance()->checkIfLeaving(node))
        {
            needToUpdate = true;
            break;
        }
    }

    DMember::instance()->updateMembers(members);
    if (needToUpdate)
    {
        std::vector<std::string> memberAddresses;
        for (const auto& member : heartbeat->memberlist())
        {
            if (!members->at(member.id())->isLeaving())
            {
                memberAddresses.push_back(member.id());
            }
        }

        // It's important to sort because we want every node to have a membership list in the same order
        // so that our algorithm for updating connections is consistent across the whole group
        std::sort(memberAddresses.begin(), memberAddresses.end());
        DMember::instance()->updateConnection(memberAddresses);
    }
}
