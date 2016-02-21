//
// Created by e-al on 05.12.15.
//

#ifndef DMEMBER_TESTBOLT_H
#define DMEMBER_TESTBOLT_H


#include <src/topology/Topology.h>

namespace topology {
class FilterBolt : public BoltBase
{
public:
    FilterBolt();

    virtual void runOnTuple(const TuplePointer &tuple, const TupleStreamPointer &outStream) override;

};
}


#endif //DMEMBER_TESTBOLT_H
