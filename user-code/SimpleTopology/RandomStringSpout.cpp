//
// Created by yura on 12/5/15.
//

#include "RandomStringSpout.h"
#include <tuple/Tuple.h>

extern "C" TopologyNodeBase *create_object()
{
    return (TopologyNodeBase *)(new RandomStringSpout);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

TuplePointer RandomStringSpout::nextTuple()
{
    TuplePointer newTuple{std::make_shared<Tuple>()};
    newTuple->id = tupleId++;
    newTuple->data.push_back("aaaaaa");

    return newTuple;
}

void RandomStringSpout::test()
{
    std::cout << "test spout" << std::endl;
}

void RandomStringSpout::runOnTuple(const TuplePointer &tuple, const TupleStreamPointer &outStream)
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "Cannot run runOnTuple() in spout" << std::endl;
}
