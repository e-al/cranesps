//
// Created by e-al on 28.11.15.
//

#ifndef DMEMBER_WORKER_H
#define DMEMBER_WORKER_H


/* Representation of Worker process, the way of communicating with bolts and spouts through RPC */

#include <Types.h>

class Worker
{
public:
    struct ConnectionGroup
    {
        ConnectionGroup() = default;
        ConnectionGroup(const std::string& newChildName
                        , const std::string& newGrouping
                        , const std::vector<std::string>& newAddresses)
        : childName(newChildName)
        , grouping(newGrouping)
        , addresses(newAddresses)
        {
        }

        std::string childName;
        std::string grouping;
        std::vector<std::string> addresses;
    };

    // asWrapped specifies whether the worker is going to act as a wrapper meaning that it will forward all the method
    // invokations to child process that runs worker executable through RPC
    Worker(const std::string &id, int dataPort, const std::string &lib, bool isBolt, bool asWrapper = true);
    virtual ~Worker();

    virtual void start();

    // stops the worker process
    virtual void kill();

    virtual void pushTuple(Tuple* tuple);
    virtual void pushTuples(const std::list<Tuple *>& tuples);
    virtual void configureConnections(const std::list<ConnectionGroup>& connectionGroups);
    void test();
    void startProcessing();

private:
    struct Impl;
    struct ChildImpl;
    struct ParentImpl;
    UniquePointer<Impl> _pimpl;
};


#endif //DMEMBER_WORKER_H
