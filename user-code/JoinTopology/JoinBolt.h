//
// Created by e-al on 05.12.15.
//

#ifndef DMEMBER_ECHOBOLT_H
#define DMEMBER_ECHOBOLT_H

#include <node/TopologyNodeBase.h>

class JoinBolt : public BoltBase
{
public:
    JoinBolt();
    virtual void runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream) override;
    virtual void test();
private:
    std::vector<std::string> _localStream;
};


#endif //DMEMBER_ECHOBOLT_H
