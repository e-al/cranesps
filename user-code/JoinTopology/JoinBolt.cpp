//
// Created by e-al on 05.12.15.
//

#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include <iterator>
#include <sstream>
#include "JoinBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new JoinBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void JoinBolt::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
    auto currentId = tuple->id;
    for (const auto& str : _localStream)
    {
        Tuple *newTuple = new Tuple(*tuple);
        newTuple->data.push_back(str);
        newTuple->id = ++currentId;
        outStream->push(newTuple);
    }

}

void JoinBolt::test()
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "test" << std::endl;
}

JoinBolt::JoinBolt()
    : _localStream{ "one", "two", "three", "four", "five" }
{

}
