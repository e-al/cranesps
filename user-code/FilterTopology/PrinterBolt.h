//
// Created by e-al on 05.12.15.
//

#ifndef DMEMBER_ECHOBOLT_H
#define DMEMBER_ECHOBOLT_H

#include <node/TopologyNodeBase.h>
#include <fstream>

class PrinterBolt : public BoltBase
{
public:
    PrinterBolt();
    virtual void runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream) override;
    virtual void test();
private:
    std::ofstream _outFile;
    TimeStamp _lastExecution;
};


#endif //DMEMBER_ECHOBOLT_H
