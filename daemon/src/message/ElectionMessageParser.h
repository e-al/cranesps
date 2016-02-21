//
// Created by e-al on 07.11.15.
//

#ifndef DMEMBER_ELECTIONMESSAGEPARSER_H
#define DMEMBER_ELECTIONMESSAGEPARSER_H

#include <Types.h>

class ElectionMessageParser
{
public:
    ElectionMessageParser() = default;
    static void parse(BufferPointer buffer, SimpleElectionMessageHandlerPointer handler);
    static void parse(SimpleBufferPointer buffer, SimpleElectionMessageHandlerPointer handler);
    static void parse(const void* buffer, int size, SimpleElectionMessageHandlerPointer handler);
};


#endif //DMEMBER_ELECTIONMESSAGEPARSER_H
