//
// Created by e-al on 29.08.15.
//

#ifndef CRANE_CLIENT_MESSAGEPARSER_H
#define CRANE_CLIENT_MESSAGEPARSER_H

#include <Types.h>

class MessageParser
{
public:
    MessageParser() = default;
    static void parse(BufferPointer buffer, SimpleMessageHandlerPointer handler);
    static void parse(SimpleBufferPointer buffer, SimpleMessageHandlerPointer handler);
    static void parse(const void* buffer, int size, SimpleMessageHandlerPointer handler);
};


#endif //CRANE_CLIENT_MESSAGEPARSER_H
