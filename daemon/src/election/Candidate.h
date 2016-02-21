//
// Created by e-al on 01.11.15.
//

#ifndef DMEMBER_CANDIDATE_H
#define DMEMBER_CANDIDATE_H


#include <Types.h>
#include <event/EventHandler.h>


class Candidate : public EventHandler
{
public:
    static Candidate* instance();

    // Start election protocol
    void start();
    void replyOk();

    // Returns a current leader id
    std::string leader();


    virtual void handle(const event::NodeFailEvent &event) override;
    virtual void handle(const event::LeaderElectedEvent& /*event*/) override {}

    int requestPort();
    void setOkReceived();
    void setNewLeader(const std::string& leaderId);
    void sendElectedConfirmation();
    void startElection();

    void onElectionConfirmationReceive();
    bool allConfirmationsReceived();

    template<typename EventT>
    void subscribe(SimpleEventHandlerPointer handler)
    {
        addListener(event::getId<EventT>(), handler);
    }

private:
    Candidate();
    struct Impl;
    UniquePointer<Impl> _pimpl;

    void addListener(int eventId, SimpleEventHandlerPointer handler);
};


#endif //DMEMBER_CANDIDATE_H
