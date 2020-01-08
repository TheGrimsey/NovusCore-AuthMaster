#include "ServiceLocator.h"

entt::registry* ServiceLocator::_mainRegistry = nullptr;
MessageHandler* ServiceLocator::_messageHandler = nullptr;