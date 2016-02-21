//
// Created by e-al on 01.11.15.
//

#ifndef DMEMBER_NODEFAILEVENT_H
#define DMEMBER_NODEFAILEVENT_H

#include "Event.h"
#include <string>

namespace event {
class NodeFailEvent
        : public Event
{
public:
    NodeFailEvent(const std::string &newId)
            : id(newId)
    { }

    std::string id;
};
}


#endif //DMEMBER_NODEFAILEVENT_H
