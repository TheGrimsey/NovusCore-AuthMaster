#include "MessageHandler.h"
#include "../ECS/Components/ConnectionComponent.h"

MessageHandler::MessageHandler()
{
    for (i32 i = 0; i < Opcode::OPCODE_MAX_COUNT; i++)
    {
        handlers[i] = nullptr;
    }
}

void MessageHandler::SetMessageHandler(Opcode opcode, MessageHandlerFn func)
{
    handlers[opcode] = func;
}

bool MessageHandler::CallHandler(Packet* packet)
{
    return handlers[packet->header.opcode] ? handlers[packet->header.opcode](packet) : true;
}