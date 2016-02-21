//
// Created by yura on 11/19/15.
//

#ifndef DMEMBER_CRANECLIENT_H
#define DMEMBER_CRANECLIENT_H

#include <topology/TopologySpec.h>
#include <Types.h>

class Client {
public:
    static Client *instance();
    virtual ~Client() = default;

    void config(const std::string &ip, int port);

    // client commands
    void submit(TopologySpec spec, const std::string &dir);
    void start(const std::string &topoid);
    void stop(const std::string &topoid);
    void kill(const std::string &topoid);
    void status();

    void send(const std::string &topoid);

private:
    Client();
    struct Impl;
    UniquePointer<Impl> _pImpl;

};


#endif //DMEMBER_CRANECLIENT_H
