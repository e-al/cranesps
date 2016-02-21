//
// Created by yura on 12/5/15.
//

#ifndef DMEMBER_RANDOMSTRINGSPOUT_H
#define DMEMBER_RANDOMSTRINGSPOUT_H

#include <node/TopologyNodeBase.h>

class RandomStringSpout : public SpoutBase
{
public:
    virtual TuplePointer nextTuple() override;
    virtual void runOnTuple(const TuplePointer &tuple, const TupleStreamPointer &outStream) override;
    virtual void test() override;
    int tupleId = 0;
};


#endif //DMEMBER_RANDOMSTRINGSPOUT_H
