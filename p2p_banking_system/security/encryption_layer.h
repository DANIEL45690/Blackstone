#ifndef ENCRYPTION_LAYER_H
#define ENCRYPTION_LAYER_H

#include "../core/aes256.h"
#include "../core/ecc.h"
#include <string>
#include <map>
#include <mutex>

class EncryptionLayer
{
    AdvancedAES256 aes;
    EllipticCurveCryptography ecc;
    std::map<std::string, std::string> sessionKeys;
    std::mutex layerMutex;
    std::string nodeId;

public:
    explicit EncryptionLayer(const std::string &id);

    bool initialize(const std::string &masterKey);
    std::string encryptForPeer(const std::string &peerId, const std::string &plaintext);
    std::string decryptFromPeer(const std::string &peerId, const std::string &ciphertext);
    bool establishPeerSession(const std::string &peerId, uint64_t peerPublicKey);
    std::string getPublicKeyString() const;
    uint64_t getPublicKey() const;

    std::string signData(const std::string &data);
    bool verifySignature(const std::string &data, const std::string &signature, uint64_t publicKey);
};

class KeyRotator
{
    std::string currentKey;
    std::string previousKey;
    uint64_t lastRotation;
    uint64_t rotationInterval;
    std::mutex rotatorMutex;

public:
    explicit KeyRotator(uint64_t intervalMs = 86400000);

    std::string getCurrentKey() const;
    std::string getPreviousKey() const;
    void rotateKey();
    bool shouldRotate() const;
    uint64_t timeUntilRotation() const;
};

#endif
