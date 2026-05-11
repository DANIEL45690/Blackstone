#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "message.h"
#include <functional>
#include <map>

class ProtocolHandler
{
    std::map<MessageType, std::function<void(const NetworkMessage &)>> handlers;
    std::mutex handlersMutex;

public:
    void registerHandler(MessageType type, std::function<void(const NetworkMessage &)> handler);
    void unregisterHandler(MessageType type);
    void handleMessage(const NetworkMessage &msg);
    bool hasHandler(MessageType type) const;
};

class ProtocolState
{
    enum class State
    {
        DISCONNECTED,
        HANDSHAKE,
        AUTHENTICATING,
        CONNECTED,
        SYNCING,
        READY
    };

    State currentState;
    std::string peerId;
    uint64_t handshakeStartTime;

public:
    ProtocolState();

    State getState() const;
    void setState(State newState);
    bool canSendMessage(MessageType type) const;
    void setPeerId(const std::string &id);
    std::string getPeerId() const;
    void reset();
};

#endif
