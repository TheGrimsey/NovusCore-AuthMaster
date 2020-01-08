#pragma once
#include "Opcodes.h"

struct NetPacket;
struct ConnectionComponent;
typedef bool (*MessageHandlerFunc)(NetPacket*, ConnectionComponent*);
class MessageHandler
{
public:
    MessageHandler();

    void SetMessageHandler(Opcode opcode, MessageHandlerFunc func);
    bool CallHandler(NetPacket* packet, ConnectionComponent* connectionComponent);

private:
    MessageHandlerFunc handlers[Opcode::OPCODE_MAX_COUNT];
};