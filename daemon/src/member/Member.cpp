// /
// Created by Tianyuan Liu on 9/16/15.
//

#include "Member.h"
#include <sstream>
#include <SystemHelper.h>

Member::Member(const MemberId &id, int sessionId, int count, TimeStamp timeStamp, bool suspicious, bool leaving)
    : _memberId(id)
    , _sessionId(sessionId)
    , _count(count)
    , _lastUpdate(timeStamp)
    , _suspicious(suspicious)
    , _leaving(leaving)
{
}

Member::~Member() { }

void Member::update(int count, TimeStamp timestamp, bool suspicious, bool leaving)
{
    this->_count = count;
    this->_lastUpdate = timestamp;
    this->_suspicious = suspicious;
    this->_leaving = leaving;
    return;
}

void Member::update(Member &member)
{
    if (member._count > this->_count)
    {
        _count = member._count;
        _lastUpdate = std::chrono::high_resolution_clock::now();
        _leaving = member._leaving;
        return;
    }
}

bool Member::isTimedOut()
{
    TimeStamp now = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdate).count();
    return duration > T_FAIL;
}

bool Member::shouldBeCleanUp()
{
    TimeStamp now = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdate).count();
    return (duration > (T_FAIL + T_CLEANUP)) && !isLeaving();
}

const MemberId &Member::id()
{
    return _memberId;
}

int Member::count()
{
    return _count;
}

const TimeStamp Member::timestamp()
{
    return _lastUpdate;
}

void Member::setTimestamp(const TimeStamp& ts)
{
    _lastUpdate = ts;
}

bool Member::isLeaving()
{
    return _leaving;
}

bool Member::hasLeft()
{
    TimeStamp now = std::chrono::high_resolution_clock::now();
    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastUpdate).count();
    return duration > T_LEFT;
}

void Member::setLeaving(bool leaving)
{
    _leaving = leaving;
}

std::string Member::toString()
{
    std::stringstream ss;
    ss << id() << '\t' << count() << '\t' << SystemHelper::timeToString(timestamp()) << '\t'
    << std::string(isLeaving() ? "Yes" : "No") << '\t'
    << std::string(isTimedOut() ? "Yes" : "No") << '\t' << std::string(shouldBeCleanUp() ? "Yes" : "No");

    return ss.str();
}
