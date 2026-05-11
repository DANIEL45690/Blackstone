#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstdint>
#include <vector>

enum class MessageType : uint8_t
{
    PING = 0x01,
    PONG = 0x02,
    TRANSACTION = 0x03,
    BLOCK = 0x04,
    BLOCKCHAIN_SYNC = 0x05,
    BLOCKCHAIN_RESPONSE = 0x06,
    PEER_DISCOVERY = 0x07,
    PEER_RESPONSE = 0x08,
    KEY_EXCHANGE = 0x09,
    KEY_EXCHANGE_RESPONSE = 0x0A,
    AUTHENTICATION = 0x0B,
    AUTHENTICATION_RESPONSE = 0x0C,
    VOTE = 0x0D,
    COMMIT = 0x0E,
    STATUS = 0x0F,
    ERROR = 0x10
};

class NetworkMessage
{
public:
    MessageType type;
    std::string senderId;
    std::string recipientId;
    std::string payload;
    uint64_t sequence;
    uint64_t timestamp;
    std::string signature;

    NetworkMessage();
    NetworkMessage(MessageType t, const std::string &sender, const std::string &recipient, const std::string &data);

    std::string serialize() const;
    static NetworkMessage deserialize(const std::string &data);
    std::string getMessageId() const;
    bool verifyIntegrity() const;
    void sign(const std::string &privateKey);
    bool verifySignature(const std::string &publicKey) const;
};

class MessageQueue
{
    std::queue<NetworkMessage> messages;
    std::mutex queueMutex;
    size_t maxSize;

public:
    explicit MessageQueue(size_t maxQueueSize = 10000);

    void push(const NetworkMessage &msg);
    bool pop(NetworkMessage &msg);
    size_t size() const;
    void clear();
    bool isEmpty() const;
};

#endif
