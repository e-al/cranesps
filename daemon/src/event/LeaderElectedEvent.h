//
// Created by e-al on 08.11.15.
//

#ifndef DMEMBER_LEADERELECTEDEVENT_H
#define DMEMBER_LEADERELECTEDEVENT_H

#include <string>
#include <event/Event.h>

namespace event {

class LeaderElectedEvent : public Event
{
public:
    LeaderElectedEvent(const std::string &newId)
            : id(newId)
    { }

    std::string id;
};

}

#endif //DMEMBER_LEADERELECTEDEVENT_H
