//
// Created by e-al on 27.11.15.
//

#ifndef DMEMBER_TOPOLOGYNODEBASE_H
#define DMEMBER_TOPOLOGYNODEBASE_H

#include <string>
#include <Types.h>

class TopologyNodeBase
{
public:

    // run user code on tuple, put result in the stream for later consumption
    virtual void runOnTuple(const Tuple* tuple, const TupleStreamPointer& outStream) = 0;
    virtual void test() = 0;
};

class BoltBase: public TopologyNodeBase
{
};

class SpoutBase: public TopologyNodeBase
{
public:

    virtual Tuple * nextTuple() = 0;
};

#endif //DMEMBER_TOPOLOGYNODEBASE_H
