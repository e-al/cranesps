//
// Created by e-al on 01.11.15.
//

#ifndef DMEMBER_EVENT_H
#define DMEMBER_EVENT_H

namespace event {
    static int baseId = 0;
    template<typename EventT>
    int getId()
    {
        static int id = baseId++;
        return id;
    }


    class Event
    {

    };

}


#endif //DMEMBER_EVENT_H
