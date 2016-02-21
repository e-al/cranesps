//
// Created by e-al on 29.08.15.
//

#ifndef DGREP_MESSAGEPARSER_H
#define DGREP_MESSAGEPARSER_H

#include <Types.h>

class DFileMessageParser
{
public:
    DFileMessageParser() = default;
    static void parse(BufferPointer buffer, SimpleDFileMessageHandlerPointer handler);
    static void parse(SimpleBufferPointer buffer, SimpleDFileMessageHandlerPointer handler);
    static void parse(const void* buffer, int size, SimpleDFileMessageHandlerPointer handler);
};


#endif //DGREP_MESSAGEPARSER_H
