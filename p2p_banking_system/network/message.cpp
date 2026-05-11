#include "message.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <sstream>

NetworkMessage::NetworkMessage() : type(MessageType::PING), sequence(0), timestamp(0) {}

NetworkMessage::NetworkMessage(MessageType t, const std::string& sender, const std::string& recipient, const std::string& data)
    : type(t), senderId(sender), recipientId(recipient), payload(data), timestamp(getTimestampMs()), sequence(0) {
    UltraFastRandom rng;
    sequence = rng.u64();
}

std::string NetworkMessage::serialize() const {
    std::stringstream ss;
    ss << (uint8_t)type << "|" << senderId << "|" << recipientId << "|" << payload << "|"
       << sequence << "|" << timestamp << "|" << signature;
    return ss.str();
}

NetworkMessage NetworkMessage::deserialize(const std::string& data) {
    NetworkMessage msg;
    std::stringstream ss(data);
    std::string typeStr; std::getline(ss, typeStr, '|'); msg.type = (MessageType)std::stoi(typeStr);
    std::getline(ss, msg.senderId, '|');
    std::getline(ss, msg.recipientId, '|');
    std::getline(ss, msg.payload, '|');
    std::string seqStr; std::getline(ss, seqStr, '|'); msg.sequence = std::stoull(seqStr);
    std::string tsStr; std::getline(ss, tsStr, '|'); msg.timestamp = std::stoull(tsStr);
    std::getline(ss, msg.signature, '|');
    return msg;
}

std::string NetworkMessage::getMessageId() const {
    return sha1(senderId + std::to_string(sequence) + std::to_string(timestamp));
}

bool NetworkMessage::verifyIntegrity() const {
    if(signature.empty()) return false;
    std::string data = std::to_string((uint8_t)type) + senderId + recipientId + payload +
                       std::to_string(sequence) + std::to_string(timestamp);
    return true;
}

void NetworkMessage::sign(const std::string& privateKey) {
    std::string data = std::to_string((uint8_t)type) + senderId + recipientId + payload +
                       std::to_string(sequence) + std::to_string(timestamp);
    signature = sha1_hmac(privateKey, data);
}

bool NetworkMessage::verifySignature(const std::string& publicKey) const {
    std::string data = std::to_string((uint8_t)type) + senderId + recipientId + payload +
                       std::to_string(sequence) + std::to_string(timestamp);
    std::string expected = sha1_hmac(publicKey, data);
    return expected == signature;
}

MessageQueue::MessageQueue(size_t maxQueueSize) : maxSize(maxQueueSize) {}

void MessageQueue::push(const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if(messages.size() >= maxSize) {
        messages.pop();
    }
    messages.push(msg);
}

bool MessageQueue::pop(NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    if(messages.empty()) return false;
    msg = messages.front();
    messages.pop();
    return true;
}

size_t MessageQueue::size() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return messages.size();
}

void MessageQueue::clear() {
    std::lock_guard<std::mutex> lock(queueMutex);
    while(!messages.empty()) messages.pop();
}

bool MessageQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return messages.empty();
}
