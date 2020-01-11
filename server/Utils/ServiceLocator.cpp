#include "ServiceLocator.h"
#include "../Networking/MessageHandler.h"

entt::registry* ServiceLocator::_mainRegistry = nullptr;
MessageHandler* ServiceLocator::_clientMessageHandler = nullptr;
MessageHandler* ServiceLocator::_internalMessageHandler = nullptr;

void ServiceLocator::SetMainRegistry(entt::registry* registry)
{
    assert(_mainRegistry == nullptr);
    _mainRegistry = registry;
}
void ServiceLocator::SetClientMessageHandler(MessageHandler* clientMessageHandler)
{
    assert(_clientMessageHandler == nullptr);
    _clientMessageHandler = clientMessageHandler;
}
void ServiceLocator::SetInternalMessageHandler(MessageHandler* serverMessageHandler)
{
    assert(_internalMessageHandler == nullptr);
    _internalMessageHandler = serverMessageHandler;
}