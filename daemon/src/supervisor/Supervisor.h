//
// Created by e-al on 27.11.15.
//

#ifndef DMEMBER_SUPERVISOR_H
#define DMEMBER_SUPERVISOR_H


#include <Types.h>
#include <worker/Worker.h>

class Supervisor
{
public:
    static Supervisor *instance();

    void start();
    void addWorker(const std::string &topoId, const std::string &runName, const std::string &name
                   , int parallelismLevel);
    void startWorker(const std::string& topoId, const std::string& name);
    void killWorker(const std::string& topoId, const std::string& name);
    void configureConnections(const std::string& topoId, const std::string& name
                              , const std::list<Worker::ConnectionGroup>& connectionGroups);
    void replyConfigureConnections();
    int port();
private:
    Supervisor();

    struct Impl;
    UniquePointer<Impl> _pimpl;
};


#endif //DMEMBER_SUPERVISOR_H
