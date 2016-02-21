//
// Created by yura on 11/29/15.
//

#include "Topology.h"
#include <Types.h>
#include <crane.pb.h>
#include <SystemHelper.h>

using namespace topology;

Node::Node()
    : id("")
    , runname("")
    , paraLevel(0)
{

}

std::map<std::string, Node *> Node::nodeTypeMap;

void Node::setId(const std::string &id) {
    this->id = id;
}

void Node::setRunname(const std::string &runname) {
    this->runname = runname;
}

void Node::setParaLevel(int paraLevel) {
    this->paraLevel = paraLevel;
}

Node *Node::setParent(const std::string &id, GroupingScheme groupingScheme) {
    // does not check if this parent already exist
    nodeTypeMap[id]->children.push_back(std::pair<std::string, GroupingScheme>(this->id, groupingScheme));
    return this;
}

std::string Node::getGroupingString(GroupingScheme gs)
{
    switch (gs)
    {
        case SHUFFLE:
        {
            return "ShuffleGrouping";
        }
        case ALL:
        {
            return "AllGrouping";
        }
        case FIELD:
        {
            return "FieldGrouping";
        }
        default:
            return "DefaultGrouping";
    }
}

void Node::showNode(const std::string &id)
{
    static std::set<std::string> cache;
    auto node = nodeTypeMap[id];
    if (id == "root") cache.clear();
    else {
        if (!cache.count(id))
        {
            cache.insert(id);
            std::cout << "ID: " << id << " Runname: " << node->runname << std::endl;
            std::cout << "Specs: " << std::endl;
            std::cout << "\tParallel level: " << node->paraLevel << std::endl;
            std::cout << "\tChildrens: ";
            for (auto child : node->children)
            {
                std::cout << " " << child.first <<"(";
                switch(child.second)
                {
                    case SHUFFLE:
                    {
                        std::cout << "ShuffleGrouping";
                        break;
                    }

                    case DEFAULT:
                    {
                        std::cout << "DefaultGrouping";
                        break;
                    }
                    default:
                    {
                        std::cout << "Unknown";
                        break;
                    }

                }
                std::cout << ")";
            }
            std::cout << std::endl << std::endl;
        }

    }
    for (auto child : node->children)
    {
        showNode(child.first);
    }
}

GroupingScheme Node::parseGroupingString(std::string s)
{
    if (s == "ShuffleGrouping")
        return SHUFFLE;
    if (s == "FieldGrouping")
        return FIELD;

}

TopologyBuilder::TopologyBuilder() {
    virtualRoot = new Node();
    Node::nodeTypeMap["root"] = virtualRoot;
}

Node *TopologyBuilder::setSpout(const std::string &id, const std::string &runname) {
    auto ret = setNode(id, runname, 1);
    ret->setParent("root");
    return ret;
}

Node *TopologyBuilder::setBolt(const std::string &id, const std::string &runname, int paraLevel) {
    return setNode(id, runname, paraLevel);
}

Node *TopologyBuilder::setNode(const std::string &id, const std::string &runname, int paraLevel) {
    if (nodeIdExist(id))
    {
        std::string msg = "node id: " + id + " existed";
        throw std::invalid_argument(msg);
    }
    auto node = new Node();
    node->setId(id);
    node->setRunname(runname);
    node->setParaLevel(paraLevel);
    Node::nodeTypeMap[id] = node;
    return node;
}

bool TopologyBuilder::nodeIdExist(const std::string &id) {
    return Node::nodeTypeMap.find(id) != Node::nodeTypeMap.end();
}


void TopologyBuilder::showTopology()
{
    Node::showNode("root");
}

TopologySpec TopologyBuilder::buildTopology()
{
    TopologySpec ts;
    ts.name = name;
    std::map<std::string, SpoutSpec> spoutMap;
    std::map<std::string, BoltSpec> boltMap;
    std::queue<std::string> queue;
    for(auto child : virtualRoot->children)
    {
        queue.push(child.first);
        SpoutSpec ss;
        auto spout = Node::nodeTypeMap[child.first];
        ss.name = spout->id;
        ss.runname = spout->runname;
        ts.spoutSpecs.push_back(ss);
        spoutMap[child.first] = ss;
    }

    while(!queue.empty())
    {
        auto id = queue.front();
        queue.pop();
        auto parent = Node::nodeTypeMap[id];
        for (auto child : parent->children)
        {
            if (boltMap.find(child.first) == boltMap.end())
            {
                auto bolt = Node::nodeTypeMap[child.first];
                BoltSpec bs;
                bs.name = bolt->id;
                bs.runname = bolt->runname;
                bs.paraLevel = bolt->paraLevel;
                bs.parents.push_back(std::make_pair(parent->id, Node::getGroupingString(child.second)));
                boltMap[child.first] = bs;
                queue.push(child.first);
            }
            else
            {
                boltMap[child.first].parents.push_back(std::make_pair(parent->id, Node::getGroupingString(child.second)));
            }
        }
    }
    for (auto it : boltMap)
    {
        ts.boltSpecs.push_back(it.second);
    }

    return ts;
}