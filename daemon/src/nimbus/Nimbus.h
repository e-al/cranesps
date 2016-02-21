//
// Created by yura on 11/20/15.
//

#ifndef DMEMBER_NIMBUS_H
#define DMEMBER_NIMBUS_H


#include <Types.h>
#include <topology/TopologySpec.h>
#include <src/event/EventHandler.h>

class Nimbus : public EventHandler {
public:
    ~Nimbus() = default;

    static Nimbus *instance();
    void start();
    void reply(const CraneWrappedMessagePointer &msg);


    virtual void handle(const event::NodeFailEvent &event) override;
    virtual void handle(const event::LeaderElectedEvent &event) override ;

    //operations on TopologyTracker
    std::vector<std::string> getAllTopoIds();
    std::string getTopoStatus(const std::string &topoid);
    void setTopoStatus(const std::string &topoid, const std::string &status);
    TopologySpec getTopoSpec(const std::string &topoid);
    void setTopoSpec(const std::string &topoid, const TopologySpec &spec);
    std::vector<std::string> getTopoTaskInstances(const std::string &topoid, const std::string &runname);
    void setTopoTaskInstances(const std::string &topoid, const std::string &runname, const std::vector<std::string> &instances);
    void addTopoTaskInstance(const std::string &topoid, const std::string &name, const std::string &instance);

    void compileTopology(const std::string &topoid, const std::vector<std::string> &sources);
    void prepareTopology(const std::string &topoid);
    void startTopology(const std::string &topoid);
    void stopTopology(const std::string &topoid);
    void killTopology(const std::string &topoid);


private:
    Nimbus();
    struct Impl;
    UniquePointer<Impl> _pImpl;

};


#endif //DMEMBER_NIMBUS_H
