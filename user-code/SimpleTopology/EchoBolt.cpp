//
// Created by e-al on 05.12.15.
//

#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include "EchoBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new EchoBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void EchoBolt::runOnTuple(const TuplePointer &tuple, const TupleStreamPointer &outStream)
{
    std::cout << tuple->data[0] << "!!!" << std::endl;
    outStream->push(tuple);
    outStream->back()->data[0].append("!!!");
}

void EchoBolt::test()
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "test" << std::endl;
}

EchoBolt::EchoBolt()
{

}
