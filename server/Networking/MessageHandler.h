#pragma once
#include "Opcodes.h"

struct Packet;
class MessageHandler
{
typedef bool (*MessageHandlerFn)(Packet*);

public:
    MessageHandler();

    void SetMessageHandler(Opcode opcode, MessageHandlerFn func);
    bool CallHandler(Packet* packet);

private:
    MessageHandlerFn handlers[Opcode::OPCODE_MAX_COUNT];
};