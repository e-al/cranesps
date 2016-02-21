//
// Created by e-al on 05.12.15.
//

#ifndef DMEMBER_ECHOBOLT_H
#define DMEMBER_ECHOBOLT_H

#include <node/TopologyNodeBase.h>
#include <map>

class WordCountBolt : public BoltBase
{
public:
    WordCountBolt();
    virtual ~WordCountBolt();
    virtual void runOnTuple(const Tuple *tuple, const TupleStreamPointer &outStream) override;
    virtual void test();
private:
    std::map<std::string, int> counts;
};


#endif //DMEMBER_ECHOBOLT_H
