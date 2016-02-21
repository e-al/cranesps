//
// Created by e-al on 05.12.15.
//

#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include <iterator>
#include <sstream>
#include "SplitSentenceBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new EchoBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void EchoBolt::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
    std::stringstream iss(tuple->data[0]);
    std::vector<std::string> tokens;
    std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>()
              , std::back_inserter<std::vector<std::string>>(tokens));
    auto currentId = tuple->id;
    for (const auto& token : tokens)
    {
        Tuple *newTuple = new Tuple;
        newTuple->id = ++currentId;
        newTuple->data.push_back(token);
        outStream->push(newTuple);
    }
//    std::cout << tuple->data[0] << "!!!" << std::endl;
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
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
