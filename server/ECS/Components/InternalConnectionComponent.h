#pragma once
#include <NovusTypes.h>
#include <Networking/Connection.h>
#include <Networking/Packet.h>
#include <Utils/ConcurrentQueue.h>

struct InternalConnectionComponent
{
    InternalConnectionComponent() : packetQueue(256) { }

    std::shared_ptr<Connection> connection;
    moodycamel::ConcurrentQueue<Packet*> packetQueue;
};