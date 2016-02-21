//
// Created by e-al on 28.11.15.
//


#include <easylogging++.h>
#include <crane.pb.h>
#include <algorithm>
#include <tuple/Tuple.h>
#include <worker/Worker.h>
#include "WorkerMessageHandler.h"


void WorkerMessageHandler::process(const CraneTuplePointer &tuple)
{
//    auto tuplePtr = std::make_shared<Tuple>();
    Tuple *newTuple = new Tuple;
    newTuple->id = tuple->id();
    std::copy(tuple->data().begin(), tuple->data().end(), std::back_inserter(newTuple->data));

    worker->pushTuple(newTuple);
}

void WorkerMessageHandler::process(const CraneTupleBatchPointer &batch)
{
//    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
//    TupleStreamPointer tuples{std::make_shared<TupleStream>()};
    std::list<Tuple *> tuples;
    for (const auto& craneTuple : batch->tuples())
    {
//        TuplePointer tuple{std::make_shared<Tuple>()};
        Tuple *tuple = new Tuple;
        tuple->id = craneTuple.id();
        std::copy(craneTuple.data().begin(), craneTuple.data().end(), std::back_inserter(tuple->data));

        tuples.push_back(tuple);
    }

    worker->pushTuples(tuples);
//    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void WorkerMessageHandler::process(const CraneSupervisorConfigureConnectionsRequestPointer &connections)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    std::list<Worker::ConnectionGroup> connectionGroups;
    for (const auto& groupedConnections : connections->configureconnectionsrequest())
    {
        connectionGroups.push_back(Worker::ConnectionGroup());
        connectionGroups.back().childName = groupedConnections.childname();
        connectionGroups.back().grouping = groupedConnections.grouping();
        std::copy(groupedConnections.addresses().begin(), groupedConnections.addresses().end()
                  , std::back_inserter(connectionGroups.back().addresses));
    }

    std::cout << "About to call child Worker::configureConnections" << std::endl;
    std::cout << "Connection group size " << connectionGroups.size() << std::endl;
    worker->test();
    worker->configureConnections(connectionGroups);
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void WorkerMessageHandler::process(const CraneSupervisorStartRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    worker->startProcessing();
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}

void WorkerMessageHandler::process(const CraneSupervisorKillRequestPointer &request)
{
    LOG(DEBUG) << "Enter " << __PRETTY_FUNCTION__;
    worker->kill();
    LOG(DEBUG) << "Exit " << __PRETTY_FUNCTION__;
}
