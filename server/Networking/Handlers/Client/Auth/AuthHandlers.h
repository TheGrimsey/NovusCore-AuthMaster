#pragma once

struct Packet;
struct ConnectionComponent;
class AuthHandlers
{
    static void HandshakeHandler(Packet*, ConnectionComponent*);
};