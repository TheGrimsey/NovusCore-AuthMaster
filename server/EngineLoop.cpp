#include "EngineLoop.h"
#include <thread>
#include <iostream>
#include <Utils/Timer.h>
#include "Utils/ServiceLocator.h"
#include <Networking/InputQueue.h>
#include <Networking/Connection.h>
#include "Networking/MessageHandler.h"
#include <tracy/Tracy.hpp>

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"

// Components
#include "ECS/Components/ConnectionComponent.h"
#include "ECS/Components/InternalConnectionComponent.h"

// Systems
#include "ECS/Systems/PacketHandlerSystem.h"
#include "ECS/Systems/InternalPacketHandlerSystem.h"

// Handlers
#include "Networking/Handlers/Server/GeneralHandlers.h"

EngineLoop::EngineLoop(f32 targetTickRate)
    : _isRunning(false), _inputQueue(256), _outputQueue(256)
{
    _targetTickRate = targetTickRate;
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Start()
{
    if (_isRunning)
        return;

    // Setup Network Lib
    InputQueue::SetInputQueue(&_inputQueue);

    std::thread thread = std::thread(&EngineLoop::Run, this);
    thread.detach();
}

void EngineLoop::Stop()
{
    if (!_isRunning)
        return;

    Message message;
    message.code = MSG_IN_EXIT;
    PassMessage(message);
}

void EngineLoop::PassMessage(Message& message)
{
    _inputQueue.enqueue(message);
}

bool EngineLoop::TryGetMessage(Message& message)
{
    return _outputQueue.try_dequeue(message);
}

void EngineLoop::Run()
{
    _isRunning = true;

    SetupUpdateFramework();
    _updateFramework.registry.create();

    TimeSingleton& timeSingleton = _updateFramework.registry.set<TimeSingleton>();
    timeSingleton.deltaTime = 1.0f;

    Message setupCompleteMessage;
    setupCompleteMessage.code = MSG_OUT_SETUP_COMPLETE;
    _outputQueue.enqueue(setupCompleteMessage);

    Timer timer;
    f32 targetDelta = 1.0f / _targetTickRate;
    while (true)
    {
        f32 deltaTime = timer.GetDeltaTime();
        timer.Tick();

        timeSingleton.lifeTimeInS = timer.GetLifeTime();
        timeSingleton.lifeTimeInMS = timeSingleton.lifeTimeInS * 1000;
        timeSingleton.deltaTime = deltaTime;

        if (!Update())
            break;

        {
            ZoneScopedNC("WaitForTickRate", tracy::Color::AntiqueWhite1)

            // Wait for tick rate, this might be an overkill implementation but it has the even tickrate I've seen - MPursche
            {
                ZoneScopedNC("Sleep", tracy::Color::AntiqueWhite1) for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta - 0.0025f; deltaTime = timer.GetDeltaTime())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            {
                ZoneScopedNC("Yield", tracy::Color::AntiqueWhite1) for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta; deltaTime = timer.GetDeltaTime())
                {
                    std::this_thread::yield();
                }
            }
        }

        FrameMark
    }

    // Clean up stuff here

    Message exitMessage;
    exitMessage.code = MSG_OUT_EXIT_CONFIRM;
    _outputQueue.enqueue(exitMessage);
}

bool EngineLoop::Update()
{
    ZoneScopedNC("Update", tracy::Color::Blue2)
    {
        ZoneScopedNC("HandleMessages", tracy::Color::Green3)
            Message message;

        while (_inputQueue.try_dequeue(message))
        {
            if (message.code == -1)
                assert(false);

            if (message.code == MSG_IN_EXIT)
            {
                return false;
            }
            else if (message.code == MSG_IN_PING)
            {
                ZoneScopedNC("Ping", tracy::Color::Green3)
                    Message pongMessage;
                pongMessage.code = MSG_OUT_PRINT;
                pongMessage.message = new std::string("PONG!");
                _outputQueue.enqueue(pongMessage);
            }
            else if (message.code == MSG_IN_NET_PACKET)
            {
                Packet* packet = reinterpret_cast<Packet*>(message.objects[0]);
                ConnectionComponent* connectionComponent = nullptr;

                if (u64 identity = packet->connection->GetIdentity())
                {
                    entt::entity entity = static_cast<entt::entity>(identity);
                    connectionComponent = &_updateFramework.registry.get<ConnectionComponent>(entity);
                }
                else
                {
                    entt::entity entity = _updateFramework.registry.create();
                    connectionComponent = &_updateFramework.registry.assign<ConnectionComponent>(entity);
                    connectionComponent->connection = std::make_shared<Connection>(*packet->connection);

                    u64 entityId = entt::to_integer(entity);
                    packet->connection->SetIdentity(entityId);
                }
;
                connectionComponent->packetQueue.enqueue(packet);
            }
            else if (message.code == MSG_IN_INTERNAL_NET_PACKET)
            {
                Packet* packet = reinterpret_cast<Packet*>(message.objects[0]);
                InternalConnectionComponent* internalConnectionComponent = nullptr;

                if (u64 identity = packet->connection->GetIdentity())
                {
                    entt::entity entity = static_cast<entt::entity>(identity);
                    internalConnectionComponent = &_updateFramework.registry.get<InternalConnectionComponent>(entity);
                }
                else
                {
                    entt::entity entity = _updateFramework.registry.create();
                    internalConnectionComponent = &_updateFramework.registry.assign<InternalConnectionComponent>(entity);
                    internalConnectionComponent->connection = std::make_shared<Connection>(*packet->connection);

                    u64 entityId = entt::to_integer(entity);
                    packet->connection->SetIdentity(entityId);
                }

                internalConnectionComponent->packetQueue.enqueue(packet);
            }
            else if (message.code == MSG_IN_NET_DISCONNECT)
            {
                u64 identity = *reinterpret_cast<u64*>(message.objects[0]);
                if (identity)
                {
                    entt::entity entity = static_cast<entt::entity>(identity);
                    _updateFramework.registry.destroy(entity);
                }

                delete message.objects[0];
            }
        }
    }

    UpdateSystems();
    return true;
}

void EngineLoop::SetupUpdateFramework()
{
    tf::Framework& framework = _updateFramework.framework;
    entt::registry& registry = _updateFramework.registry;

    ServiceLocator::SetMainRegistry(&registry);
    SetupClientMessageHandler();
    SetupServerMessageHandler();

    // Temporary fix to allow taskflow to run multiple tasks at the same time when using Entt to construct views
    registry.prepare<ConnectionComponent>();
    registry.prepare<InternalConnectionComponent>();

    // PacketHandlerSystem
    tf::Task packetHandlerSystemTask = framework.emplace([&registry]()
    {
        ZoneScopedNC("PacketHandlerSystem::Update", tracy::Color::Orange2)
        PacketHandlerSystem::Update(registry);
    });

    // InternalPacketHandlerSystem
    tf::Task internalPacketHandlerSystemTask = framework.emplace([&registry]()
    {
        ZoneScopedNC("InternalPacketHandlerSystem::Update", tracy::Color::Blue2)
        InternalPacketHandlerSystem::Update(registry);
    });
}
void EngineLoop::SetupClientMessageHandler()
{
    auto messageHandler = new MessageHandler();



    ServiceLocator::SetClientMessageHandler(messageHandler);
}
void EngineLoop::SetupServerMessageHandler()
{
    auto messageHandler = new MessageHandler();
    ServiceLocator::SetInternalMessageHandler(messageHandler);

    Server::GeneralHandlers::Setup(messageHandler);
}
void EngineLoop::UpdateSystems()
{
    ZoneScopedNC("UpdateSystems", tracy::Color::Blue2)
    {
        ZoneScopedNC("Taskflow::Run", tracy::Color::Blue2)
            _updateFramework.taskflow.run(_updateFramework.framework);
    }
    {
        ZoneScopedNC("Taskflow::WaitForAll", tracy::Color::Blue2)
            _updateFramework.taskflow.wait_for_all();
    }
}
