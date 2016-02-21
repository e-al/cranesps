//
// Created by e-al on 07.11.15.
//

#include <election/Candidate.h>
#include <DMember.h>
#include "ElectionMessageHandler.h"

void ElectionMessageHandler::process(const ElectionStartPointer& /*request*/)
{
    Candidate::instance()->replyOk();
    Candidate::instance()->startElection();
}

void ElectionMessageHandler::process(const ElectionOkMessagePointer& /*okMessage*/)
{
    Candidate::instance()->setOkReceived();
}

void ElectionMessageHandler::process(const ElectionNewLeaderPointer &newLeader)
{
    if (newLeader->id() != DMember::instance()->id())
    {
        Candidate::instance()->setNewLeader(newLeader->id());
    }
    Candidate::instance()->sendElectedConfirmation();
//    Candidate::instance()->setElectionFinished();
}

void ElectionMessageHandler::process(const ElectionConfirmPointer& /*confirm*/)
{
    Candidate::instance()->onElectionConfirmationReceive();
    if (Candidate::instance()->allConfirmationsReceived())
    {
        Candidate::instance()->setNewLeader(DMember::instance()->id());
    }

}
