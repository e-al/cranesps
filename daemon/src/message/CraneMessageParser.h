//
// Created by yura on 11/20/15.
//

#ifndef DMEMBER_NIMBUSMESSAGEPARSER_H
#define DMEMBER_NIMBUSMESSAGEPARSER_H


#include <Types.h>

class CraneMessageParser {
public:
    CraneMessageParser() = default;
    static void parse(BufferPointer buffer, SimpleCraneMessageHandlerPointer handler);
    static void parse(SimpleBufferPointer buffer, SimpleCraneMessageHandlerPointer handler);
    static void parse(const void* buffer, int size, SimpleCraneMessageHandlerPointer handler);
};


#endif //DMEMBER_NIMBUSMESSAGEPARSER_H
