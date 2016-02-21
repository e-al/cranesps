//
// Created by e-al on 05.12.15.
//

#ifndef DMEMBER_ECHOBOLT_H
#define DMEMBER_ECHOBOLT_H

#include <node/TopologyNodeBase.h>

class EchoBolt : public BoltBase
{
public:
    EchoBolt();
    virtual void runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream) override;
    virtual void test();
};


#endif //DMEMBER_ECHOBOLT_H
