//
// Created by Tianyuan Liu on 9/16/15.
//

#ifndef DMEMBER_MEMBERLIST_H
#define DMEMBER_MEMBERLIST_H

#include <Types.h>

class Member {
public:
    Member(const MemberId &id, int sessionId, int count = 0
           , TimeStamp timeStamp = std::chrono::high_resolution_clock::now()
           , bool suspicious = false, bool leaving = false);
    ~Member();

    // update this member record with heartbeating information
    void update(int count, TimeStamp timestamp, bool suspicious = false, bool leaving = false);
    // update this member record with another member record
    void update(Member &);

    bool isTimedOut();
    bool shouldBeCleanUp();
    bool isLeaving();
    bool hasLeft();

    void setTimestamp(const TimeStamp& ts);
    void setLeaving(bool leaving);

    int count();
    const MemberId& id();
    const TimeStamp timestamp();
    std::string toString();

    const int T_FAIL = 2000; // T_FAIL in milliseconds
    const int T_CLEANUP = 2000; //T_CLEANUP in milliseconds
    const int T_LEFT = 4000; //T_LEFT in milliseconds

private:
    MemberId _memberId;
    int _sessionId;
    int _count;
    TimeStamp _lastUpdate;
    // Note:
    // to parse TimeStamp to int64: _lastUpdate.time_since_epoch();
    // to construct a TimeStamp from int64: auto now = Timestamp() + std::chrono::nanoseconds(int64);

    bool _suspicious;
    bool _leaving;
};


#endif //DMEMBER_MEMBERLIST_H
