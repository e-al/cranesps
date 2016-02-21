//
// Created by e-al on 05.12.15.
//

#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include <iterator>
#include <sstream>
#include "FilterBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new FilterBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void FilterBolt::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
    if (tuple->data[0].find(_filterWord) != std::string::npos)
    {
        Tuple *newTuple = new Tuple(*tuple);
        newTuple->id = tuple->id + 1;
        outStream->push(newTuple);
    }
}

void FilterBolt::test()
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "test" << std::endl;
}

FilterBolt::FilterBolt()
    : _filterWord{ "Alice" }
{

}
