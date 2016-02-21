//
// Created by e-al on 05.12.15.
//

#include <tuple/Tuple.h>
#include "FilterBolt.h"

extern "C" TopologyNodeBase *create_object()
{
    return static_cast<TopologyNodeBase *>(new topology::FilterBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

namespace topology {
void FilterBolt::runOnTuple(const TuplePointer &tuple, const TupleStreamPointer &outStream)
{
    outStream->push(tuple);
    outStream->back()->data[0].append("!!!");
}

FilterBolt::FilterBolt()
{

}
}
