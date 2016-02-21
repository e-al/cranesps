//
// Created by yura on 12/5/15.
//

#ifndef DMEMBER_RANDOMSTRINGSPOUT_H
#define DMEMBER_RANDOMSTRINGSPOUT_H

#include <node/TopologyNodeBase.h>
#include <fstream>

class InputSpout : public SpoutBase
{
public:
    InputSpout();
    virtual ~InputSpout();
    virtual Tuple *nextTuple() override;
    virtual void runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream) override;
    virtual void test() override;

private:
    int _tupleId = 0;
    std::ifstream _inputFile;
};


#endif //DMEMBER_RANDOMSTRINGSPOUT_H
