#include "EngineLoop.h"
#include <thread>
#include <iostream>
#include <Utils/Timer.h>
#include "Utils/ServiceLocator.h"
#include <Networking/InputQueue.h>
#include <Networking/Connection.h>
#include <tracy/Tracy.hpp>

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"

// Components
#include "ECS/Components/ConnectionComponent.h"

// Systems
#include "ECS/Systems/PacketHandlerSystem.h"


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

    Message setupCompleteMessage;
    setupCompleteMessage.code = MSG_OUT_SETUP_COMPLETE;
    _outputQueue.enqueue(setupCompleteMessage);

    Timer timer;
    f32 targetDelta = 1.0f / _targetTickRate;
    while (true)
    {
        f32 deltaTime = timer.GetDeltaTime();
        timer.Tick();

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
                Connection* connection = reinterpret_cast<Connection*>(message.objects[0]);
                ConnectionComponent* connectionComponent = nullptr;

                if (connection->HasIdentity())
                {
                    entt::entity entity = static_cast<entt::entity>(connection->GetIdentity());
                    connectionComponent = &_updateFramework.registry.get<ConnectionComponent>(entity);
                }
                else
                {
                    entt::entity entity = _updateFramework.registry.create();

                    u64 entityId = entt::to_integer(entity);
                    connectionComponent = &_updateFramework.registry.assign<ConnectionComponent>(entity);
                    connectionComponent->connection = std::make_shared<Connection>(*connection);
                    connection->SetIdentity(entityId);
                }

                NetPacket* netPacket = reinterpret_cast<NetPacket*>(message.objects[1]);
                connectionComponent->packetQueue.enqueue(netPacket);
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
    ServiceLocator::SetMessageHandler(new MessageHandler());

    // PacketHandlerSystem
    tf::Task packetHandlerSystemTask = framework.emplace([&registry]()
    {
        ZoneScopedNC("PacketHandlerSystem", tracy::Color::Orange2)
            PacketHandlerSystem::Update(registry);
    });
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
