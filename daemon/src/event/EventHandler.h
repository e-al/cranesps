//
// Created by e-al on 07.11.15.
//

#ifndef DMEMBER_EVENTHANDLER_H
#define DMEMBER_EVENTHANDLER_H

#include <event/Event.h>
#include <event/NodeFailEvent.h>
#include "LeaderElectedEvent.h"

class EventHandler
{
public:
    virtual void handle(const event::NodeFailEvent &event) = 0;
    virtual void handle(const event::LeaderElectedEvent& event) = 0;
};


#endif //DMEMBER_EVENTHANDLER_H

