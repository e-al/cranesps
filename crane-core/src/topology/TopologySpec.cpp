//
// Created by yura on 11/29/15.
//

#include "TopologySpec.h"
#include <crane.pb.h>

void TopologySpec::showSpec()
{
    using namespace std;
    cout << "Topology name: " << name << endl;
    cout << "Spout specs: " << endl;
    for (auto ss : spoutSpecs)
    {
        cout << "name: " << ss.name << " runname: " << ss.runname << endl;
        cout << endl;
    }
    cout << "Bolt specs: " << endl;
    for (auto bs : boltSpecs)
    {
        cout << "name: " << bs.name << " runname: " << bs.runname << " paraLevel: " << bs.paraLevel << endl;
        cout << "parents: ";
        for (auto parent : bs.parents)
        {
            cout << parent.first << "(" << parent.second << ");";
        }
        cout << endl << endl;
    }
}


TopologySpec TopologySpec::parseFromRequest(const CraneClientSubmitRequestPointer &request)
{
    TopologySpec ts;
    ts.name = request->toponame();
    for (auto ss : request->spoutspecs())
    {
        SpoutSpec spoutSpec;
        spoutSpec.name = ss.name();
        spoutSpec.runname = ss.runname();
        ts.spoutSpecs.push_back(spoutSpec);
    }
    for (auto bs : request->boltspecs())
    {
        BoltSpec boltSpec;
        boltSpec.name = bs.name();
        boltSpec.runname = bs.runname();
        if (bs.has_paralevel()) boltSpec.paraLevel = bs.paralevel();
        for (auto parent : bs.parents())
        {
            boltSpec.parents.push_back(std::make_pair(parent.name(), parent.grouping()));
        }
        ts.boltSpecs.push_back(boltSpec);
    }
    if (request->has_ackerbolt()) ts.ackerbolt = request->ackerbolt();
    return ts;
}

