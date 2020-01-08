#include "MessageHandler.h"
#include "../ECS/Components/ConnectionComponent.h"

MessageHandler::MessageHandler()
{
    for (i32 i = 0; i < Opcode::OPCODE_MAX_COUNT; i++)
    {
        handlers[i] = nullptr;
    }
}

void MessageHandler::SetMessageHandler(Opcode opcode, MessageHandlerFunc func)
{
    handlers[opcode] = func;
}

bool MessageHandler::CallHandler(NetPacket* packet, ConnectionComponent* connectionComponent)
{
    return handlers[packet->opcode] ? handlers[packet->opcode](packet, connectionComponent) : true;
}