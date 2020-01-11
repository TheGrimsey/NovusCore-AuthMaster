#pragma once

class MessageHandler;
struct Packet;
namespace Server
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandshakeHandler(Packet*);
        static bool HandshakeResponseHandler(Packet*);
    };
}