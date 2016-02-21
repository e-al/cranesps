//
// Created by e-al on 05.12.15.
//

#include <chrono>
#include <iostream>
#include <iterator>
#include <memory>
#include <Types.h>
#include <tuple/Tuple.h>
#include "PrinterBolt.h"

extern "C" TopologyNodeBase *create_object()
{
  return (TopologyNodeBase *)(new PrinterBolt);
}

extern "C" void destroy_object(TopologyNodeBase *object)
{
    delete object;
}

void PrinterBolt::runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream)
{
    static bool firstTimeExecute = true;
    auto now = std::chrono::high_resolution_clock::now();
    if (firstTimeExecute)
    {
        firstTimeExecute = false;
        _lastExecution = now;
    }

    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastExecution).count();
    _outFile << "[" << tuple->data[0] << "] " << duration << std::endl;
    _outFile.flush();
}

void PrinterBolt::test()
{
//    outStream->push(tuple);
//    outStream->back()->data[0].append("!!!");
//    std::cout << tuple->data[0] << std::endl;
    std::cout << "test" << std::endl;
}

PrinterBolt::PrinterBolt()
    : _outFile("/tmp/PrinterBolt.out", std::ofstream::out)
{

}
