#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include "../core/aes256.h"
#include "../core/ecc.h"
#include "../core/quantum_signature.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <string>
#include <memory>
#include <map>

class CryptoManager
{
    std::unique_ptr<AdvancedAES256> aes;
    std::unique_ptr<EllipticCurveCryptography> ecc;
    std::unique_ptr<QuantumResistantSignature> qrs;
    std::map<std::string, std::string> sessionKeys;
    std::map<std::string, uint64_t> peerPublicKeys;
    std::string nodeId;
    std::mutex cryptoMutex;

public:
    CryptoManager(const std::string &id);
    ~CryptoManager();

    bool initialize();
    std::string encryptMessage(const std::string &message, const std::string &peerId);
    std::string decryptMessage(const std::string &ciphertext, const std::string &peerId);
    std::string signMessage(const std::string &message);
    bool verifySignature(const std::string &message, const std::string &signature, const std::string &peerId);
    bool establishSessionKey(const std::string &peerId, uint64_t peerPublicKey);
    std::string getPublicKeyString() const;
    uint64_t getPublicKey() const;
    std::string getQuantumRoot() const;
    std::string quantumSign(const std::string &msg, int leafIdx);
    bool quantumVerify(const std::string &msg, const std::string &sig, int leafIdx);
};

class KeyDerivation
{
public:
    static std::string pbkdf2(const std::string &password, const std::string &salt, int iterations, int keyLen);
    static std::string scrypt(const std::string &password, const std::string &salt, int n, int r, int p, int keyLen);
    static std::string argon2(const std::string &password, const std::string &salt, int iterations, int memory, int threads, int keyLen);
};

#endif
