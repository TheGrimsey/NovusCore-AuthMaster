#pragma once
#include <NovusTypes.h>
#include <Networking/Connection.h>
#include <Networking/NetPacket.h>
#include <Utils/ConcurrentQueue.h>

struct ConnectionComponent
{
    ConnectionComponent() : packetQueue(256) { }

    std::shared_ptr<Connection> connection;
    moodycamel::ConcurrentQueue<NetPacket*> packetQueue;
};