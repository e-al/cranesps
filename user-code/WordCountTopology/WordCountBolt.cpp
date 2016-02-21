//
// Created by e-al on 05.12.15.
//

#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include <iostream>
#include "WordCountBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new WordCountBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void WordCountBolt::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
    int count = counts[tuple->data[0]];
    counts[tuple->data[0]] = ++count;
    Tuple *newTuple = new Tuple;
    newTuple->id = tuple->id + 1;
    newTuple->data.push_back(tuple->data[0]);
    newTuple->data.push_back(std::to_string(count));
//    std::cout << "Added word count: " << newTuple->data[0] << ", " << newTuple->data[1] << std::endl;
    outStream->push(newTuple);
}

void WordCountBolt::test()
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "test" << std::endl;
}

WordCountBolt::WordCountBolt()
{

}

WordCountBolt::~WordCountBolt()
{
    counts.clear();
}
