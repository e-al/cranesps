//
// Created by yura on 11/19/15.
//

#ifndef DMEMBER_TOPOLOGYSPEC_H
#define DMEMBER_TOPOLOGYSPEC_H

#include <iostream>
#include <vector>
#include <Types.h>

class BoltSpec {
public:
    BoltSpec()
        : name("")
        , runname("run")
        , paraLevel(3) {}
    ~BoltSpec() {}

    std::string name;
    std::vector<std::pair<std::string, std::string>> parents;
    std::string runname;
    int paraLevel;

};

class SpoutSpec {
public:
    SpoutSpec()
        : name("")
        , runname("run") {}
    ~SpoutSpec() {}

    std::string name;
    std::string runname;
};

class TopologySpec {
public:
    TopologySpec()
        : name("")
        , node(3)
        , ackerbolt(true)
    {}
    ~TopologySpec(){}

    void showSpec();
    static TopologySpec parseFromRequest(const CraneClientSubmitRequestPointer &request);

    std::string name;
    std::vector<BoltSpec> boltSpecs;
    std::vector<SpoutSpec> spoutSpecs;
    int node;
    bool ackerbolt;

};

#endif //DMEMBER_TOPOLOGYSPEC_H


