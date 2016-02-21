//
// Created by Tianyuan Liu on 9/23/15.
//

#ifndef DMEMBER_INTRODUCER_H
#define DMEMBER_INTRODUCER_H

#include <Types.h>


class Introducer {
public:
    ~Introducer();

    Introducer(DMember* parent);
    void start();

    void setAvailable(const std::string &, long);
    void updateMember(const MemberPointer& member);
    void recordAndReply(const std::string &);
    void updateNodes();

    static int servicePort();

private:
    DMember *parent;
    struct Impl;
    UniquePointer<Impl> _pImpl;


};


#endif //DMEMBER_INTRODUCER_H
