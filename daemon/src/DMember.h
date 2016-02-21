//
// Created by Tianyuan Liu on 9/16/15.
//

#ifndef DMEMBER_DMEMBER_H
#define DMEMBER_DMEMBER_H

#include <Types.h>
#include <event/Event.h>
#include <event/EventHandler.h>
#include "introducer/Introducer.h"
#include <list>

//struct ImplBase
//{
//    virtual void addListener(int id, const EventHandler& handler) = 0;
//};

class DMember {
public:
    virtual ~DMember();
    static DMember *instance();

    void start(bool asIntroducer=false);
    void stop();

    Introducer* introducer;

    void replyPingRequest();
    void updateJoinTime(long);
    void updateConnection(const std::vector<std::string> &);
    void updateMembers(const MemberMapPointer &members);

    // Used by the introducer, called when a new node joins the group.
    // The idea is to update connections on all nodes whenever someone joins.
    // Since everybody is listening to the introducer, it's convenient to use heartbeats with update flag for this purpose.
    void updateConnectionsOnNextHeartbeat(const MemberMapPointer &members);
    bool checkIfLeaving(const MemberId& id);

    template<typename EventT>
    void subscribe(SimpleEventHandlerPointer handler)
    {
        addListener(event::getId<EventT>(), handler);
    }


    std::string joinTime();
    std::string id();
    std::list<std::string> membershipList();
    std::list<std::string> membershipListInfo();


    int pingPort();

private:
    DMember();
    struct Impl;
    UniquePointer<Impl> _pImpl;

    void addListener(int eventId, SimpleEventHandlerPointer handler);
};


#endif //DMEMBER_DMEMBER_H
