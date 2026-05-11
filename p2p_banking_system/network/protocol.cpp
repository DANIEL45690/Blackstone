#include "protocol.h"

void ProtocolHandler::registerHandler(MessageType type, std::function<void(const NetworkMessage &)> handler)
{
    std::lock_guard<std::mutex> lock(handlersMutex);
    handlers[type] = handler;
}

void ProtocolHandler::unregisterHandler(MessageType type)
{
    std::lock_guard<std::mutex> lock(handlersMutex);
    handlers.erase(type);
}

void ProtocolHandler::handleMessage(const NetworkMessage &msg)
{
    std::lock_guard<std::mutex> lock(handlersMutex);
    auto it = handlers.find(msg.type);
    if (it != handlers.end())
    {
        it->second(msg);
    }
}

bool ProtocolHandler::hasHandler(MessageType type) const
{
    std::lock_guard<std::mutex> lock(handlersMutex);
    return handlers.find(type) != handlers.end();
}

ProtocolState::ProtocolState() : currentState(State::DISCONNECTED), handshakeStartTime(0) {}

ProtocolState::State ProtocolState::getState() const
{
    return currentState;
}

void ProtocolState::setState(State newState)
{
    currentState = newState;
}

bool ProtocolState::canSendMessage(MessageType type) const
{
    switch (currentState)
    {
    case State::DISCONNECTED:
        return false;
    case State::HANDSHAKE:
        return type == MessageType::KEY_EXCHANGE || type == MessageType::KEY_EXCHANGE_RESPONSE;
    case State::AUTHENTICATING:
        return type == MessageType::AUTHENTICATION || type == MessageType::AUTHENTICATION_RESPONSE;
    case State::CONNECTED:
        return type == MessageType::STATUS || type == MessageType::PING || type == MessageType::PONG;
    case State::SYNCING:
        return type == MessageType::BLOCKCHAIN_SYNC || type == MessageType::BLOCKCHAIN_RESPONSE;
    case State::READY:
        return true;
    default:
        return false;
    }
}

void ProtocolState::setPeerId(const std::string &id)
{
    peerId = id;
}

std::string ProtocolState::getPeerId() const
{
    return peerId;
}

void ProtocolState::reset()
{
    currentState = State::DISCONNECTED;
    peerId.clear();
    handshakeStartTime = 0;
}
