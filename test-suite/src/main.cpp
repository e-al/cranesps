//
// Created by yura on 11/20/15.
//

#include <src/topology/Topology.h>
#include <src/CraneClient.h>
#include "FilterBolt.h"

using namespace topology;


class TestSpout : public SpoutBase {

};

//class FilterBolt : public BoltBase {
//
//};

class SinkBolt : public BoltBase {

};

int main()
{
    Client *client = Client::instance();
    client->config("fa15-cs425-g10-01.cs.illinois.edu", 4444);
    std::map<std::string, std::string> path;
    path["9TestSpout"] = "path1";
    path["10FilterBolt"] = "path2";
    path["8SinkBolt"] = "path3";

    TopologyBuilder builder;
    builder.name = "testtopo";
    builder.setSpout("input", new TestSpout(), 10);
    builder.setBolt("filter", new FilterBolt(), 5)->setParent("input", SHUFFLE);
    builder.setBolt("sink", new SinkBolt(), 3)->setParent("filter", SHUFFLE)->setParent("input", SHUFFLE);
    builder.setBolt("another-filter", new FilterBolt(), 3)->setParent("filter");

    client->submit(builder.buildTopology(), path);


}