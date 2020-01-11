#include "InternalPacketHandlerSystem.h"
#include "../Components/InternalConnectionComponent.h"
#include "../../Networking/MessageHandler.h"
#include "../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void InternalPacketHandlerSystem::Update(entt::registry& registry)
{
    MessageHandler* messageHandler = ServiceLocator::GetInternalMessageHandler();
    auto view = registry.view<InternalConnectionComponent>();
    view.each([&registry, &messageHandler](const auto, InternalConnectionComponent& internalConnectionComponent)
    {
            ZoneScopedNC("InternalPacketHandlerSystem::Update", tracy::Color::Blue)

                Packet* packet;
            while (internalConnectionComponent.packetQueue.try_dequeue(packet))
            {
                if (!messageHandler->CallHandler(packet))
                {
                    internalConnectionComponent.packetQueue.enqueue(packet);
                    continue;
                }

                delete packet;
            }
    });
}