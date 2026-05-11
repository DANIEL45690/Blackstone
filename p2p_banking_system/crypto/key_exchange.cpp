#include "key_exchange.h"
#include "../core/random.h"
#include "../core/sha1.h"
#include <sstream>

KeyExchangeProtocol::KeyExchangeProtocol(const std::string &id, uint64_t pubKey, uint64_t privKey)
    : nodeId(id), myPublicKey(pubKey), myPrivateKey(privKey) {}

std::string KeyExchangeProtocol::initiateKeyExchange(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(exchangeMutex);
    UltraFastRandom rng;
    KeyExchangeState state;
    state.peerId = peerId;
    state.publicKey = 0;
    state.ephemeralKey = rng.u64();
    state.sessionId = sha1(peerId + std::to_string(state.ephemeralKey) + std::to_string(getTimestampMs()));
    state.completed = false;
    state.timestamp = getTimestampMs();
    activeExchanges[peerId] = state;
    return serializeKeyMessage(myPublicKey, state.ephemeralKey);
}

std::string KeyExchangeProtocol::respondToKeyExchange(const std::string &peerId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(exchangeMutex);
    auto peerKeys = deserializeKeyMessage(message);
    UltraFastRandom rng;
    KeyExchangeState state;
    state.peerId = peerId;
    state.publicKey = peerKeys.first;
    state.ephemeralKey = rng.u64();
    state.sessionId = sha1(peerId + std::to_string(state.ephemeralKey) + std::to_string(peerKeys.second));
    state.completed = false;
    state.timestamp = getTimestampMs();
    activeExchanges[peerId] = state;
    return serializeKeyMessage(myPublicKey, state.ephemeralKey);
}

bool KeyExchangeProtocol::finalizeKeyExchange(const std::string &peerId, const std::string &response)
{
    std::lock_guard<std::mutex> lock(exchangeMutex);
    auto it = activeExchanges.find(peerId);
    if (it == activeExchanges.end())
        return false;
    auto peerKeys = deserializeKeyMessage(response);
    it->second.publicKey = peerKeys.first;
    it->second.completed = true;
    return true;
}

std::string KeyExchangeProtocol::getSharedSecret(const std::string &peerId) const
{
    std::lock_guard<std::mutex> lock(exchangeMutex);
    auto it = activeExchanges.find(peerId);
    if (it == activeExchanges.end() || !it->second.completed)
        return "";
    uint64_t shared = 1;
    std::stringstream ss;
    ss << std::hex << shared;
    return sha1_double(ss.str());
}

void KeyExchangeProtocol::cleanupOldExchanges(uint64_t maxAgeMs)
{
    std::lock_guard<std::mutex> lock(exchangeMutex);
    uint64_t now = getTimestampMs();
    for (auto it = activeExchanges.begin(); it != activeExchanges.end();)
    {
        if (now - it->second.timestamp > maxAgeMs)
        {
            it = activeExchanges.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::string KeyExchangeProtocol::serializeKeyMessage(uint64_t publicKey, uint64_t nonce)
{
    std::stringstream ss;
    ss << std::hex << publicKey << ":" << std::hex << nonce;
    return ss.str();
}

std::pair<uint64_t, uint64_t> KeyExchangeProtocol::deserializeKeyMessage(const std::string &msg)
{
    size_t colon = msg.find(':');
    if (colon == std::string::npos)
        return {0, 0};
    uint64_t pubKey = std::stoull(msg.substr(0, colon), nullptr, 16);
    uint64_t nonce = std::stoull(msg.substr(colon + 1), nullptr, 16);
    return {pubKey, nonce};
}
