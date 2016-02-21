//
// Created by yura on 12/5/15.
//

#include "InputSpout.h"
#include <tuple/Tuple.h>
#include <chrono>
#include <thread>

extern "C" TopologyNodeBase *create_object()
{
    return (TopologyNodeBase *)(new InputSpout);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

InputSpout::InputSpout()
    : _inputFile("/tmp/wctest.txt")
{
}

InputSpout::~InputSpout()
{
    _inputFile.close();
}

Tuple * InputSpout::nextTuple()
{
//    TuplePointer newTuple{std::make_shared<Tuple>()};
    Tuple *newTuple = new Tuple;
    newTuple->id = _tupleId++;
    std::string line;
    std::getline(_inputFile, line);
    newTuple->data.push_back(line);

    return newTuple;
}

void InputSpout::test()
{
    std::cout << "test spout" << std::endl;
}

void InputSpout::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "Cannot run runOnTuple() in spout" << std::endl;
}
