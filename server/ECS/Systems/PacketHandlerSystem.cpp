#include "PacketHandlerSystem.h"
#include "../Components/ConnectionComponent.h"
#include "../../Networking/MessageHandler.h"
#include "../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void PacketHandlerSystem::Update(entt::registry& registry)
{
    MessageHandler* messageHandler = ServiceLocator::GetMessageHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &messageHandler](const auto, ConnectionComponent& connectionComponent)
    {
            ZoneScopedNC("PacketHandlerSystem::Update", tracy::Color::Blue)

                NetPacket* packet;
            while (connectionComponent.packetQueue.try_dequeue(packet))
            {
                if (!messageHandler->CallHandler(packet, &connectionComponent))
                {
                    connectionComponent.packetQueue.enqueue(packet);
                    continue;
                }

                delete packet;
            }
    });
}