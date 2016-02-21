//
// Created by yura on 11/17/15.
//

#ifndef DMEMBER_TOPOLOGY_H
#define DMEMBER_TOPOLOGY_H

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <node/TopologyNodeBase.h>
#include "TopologySpec.h"


namespace topology {

    enum GroupingScheme {
        SHUFFLE,
        FIELD,
        ALL,
        DEFAULT
    };

    class Node
    {
        friend class TopologyBuilder;
    public:
        Node();
        ~Node() = default;
        static std::map<std::string, Node *> nodeTypeMap;

        Node *setParent(const std::string &id, GroupingScheme groupingScheme=DEFAULT);

        void setId(const std::string &id);
        void setRunname(const std::string &runname);
        void setParaLevel(int paraLevel);

        static void showNode(const std::string &id);
        static std::string getGroupingString(GroupingScheme gs);
        static GroupingScheme parseGroupingString(std::string s);

    private:
        std::string id;
        std::string runname;
        int paraLevel;
        std::vector<std::pair<std::string, GroupingScheme>> children;
    };



    class TopologyBuilder
    {
    public:
        TopologyBuilder();
        virtual ~TopologyBuilder() = default;

        std::string name;

        Node *setSpout(const std::string &id, const std::string &runname);
        Node *setBolt(const std::string &id, const std::string &runname, int paraLevel);

        TopologySpec buildTopology();

        void showTopology(); // print topology


    private:
        Node *virtualRoot;

        Node *setNode(const std::string &id, const std::string &runname, int paraLevel);
        bool nodeIdExist(const std::string &id);
        void showTopology(Node* root);

    };
}



#endif //DMEMBER_TOPOLOGY_H
