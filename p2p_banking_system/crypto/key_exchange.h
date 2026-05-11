#ifndef KEY_EXCHANGE_H
#define KEY_EXCHANGE_H

#include <string>
#include <vector>
#include <map>
#include <mutex>

class KeyExchangeProtocol
{
    struct KeyExchangeState
    {
        std::string peerId;
        uint64_t publicKey;
        uint64_t ephemeralKey;
        std::string sessionId;
        bool completed;
        uint64_t timestamp;
    };

    std::map<std::string, KeyExchangeState> activeExchanges;
    std::mutex exchangeMutex;
    std::string nodeId;
    uint64_t myPublicKey;
    uint64_t myPrivateKey;

public:
    KeyExchangeProtocol(const std::string &id, uint64_t pubKey, uint64_t privKey);

    std::string initiateKeyExchange(const std::string &peerId);
    std::string respondToKeyExchange(const std::string &peerId, const std::string &message);
    bool finalizeKeyExchange(const std::string &peerId, const std::string &response);
    std::string getSharedSecret(const std::string &peerId) const;
    void cleanupOldExchanges(uint64_t maxAgeMs);

    static std::string serializeKeyMessage(uint64_t publicKey, uint64_t nonce);
    static std::pair<uint64_t, uint64_t> deserializeKeyMessage(const std::string &msg);
};

#endif
