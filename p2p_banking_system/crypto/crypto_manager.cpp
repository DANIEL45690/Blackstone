#include "crypto_manager.h"

CryptoManager::CryptoManager(const std::string &id) : nodeId(id)
{
    aes = std::make_unique<AdvancedAES256>();
    ecc = std::make_unique<EllipticCurveCryptography>();
    qrs = std::make_unique<QuantumResistantSignature>(12);
}

CryptoManager::~CryptoManager() {}

bool CryptoManager::initialize()
{
    std::string masterKey = sha1_double(nodeId + "MASTER_KEY");
    aes->setKey(masterKey);
    return aes->isInitialized();
}

std::string CryptoManager::encryptMessage(const std::string &message, const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(cryptoMutex);
    auto it = sessionKeys.find(peerId);
    if (it == sessionKeys.end())
        return "";
    AdvancedAES256 peerAes(it->second);
    return peerAes.encryptGCM(message);
}

std::string CryptoManager::decryptMessage(const std::string &ciphertext, const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(cryptoMutex);
    auto it = sessionKeys.find(peerId);
    if (it == sessionKeys.end())
        return "";
    AdvancedAES256 peerAes(it->second);
    return peerAes.decryptGCM(ciphertext);
}

std::string CryptoManager::signMessage(const std::string &message)
{
    auto sig = ecc->sign(message);
    std::stringstream ss;
    ss << std::hex << sig.first << ":" << std::hex << sig.second;
    return ss.str();
}

bool CryptoManager::verifySignature(const std::string &message, const std::string &signature, const std::string &peerId)
{
    auto it = peerPublicKeys.find(peerId);
    if (it == peerPublicKeys.end())
        return false;
    size_t colon = signature.find(':');
    if (colon == std::string::npos)
        return false;
    uint64_t r1 = std::stoull(signature.substr(0, colon), nullptr, 16);
    uint64_t s = std::stoull(signature.substr(colon + 1), nullptr, 16);
    return ecc->verify(message, r1, s, it->second);
}

bool CryptoManager::establishSessionKey(const std::string &peerId, uint64_t peerPublicKey)
{
    std::lock_guard<std::mutex> lock(cryptoMutex);
    peerPublicKeys[peerId] = peerPublicKey;
    uint64_t sharedSecret = ecc->computeSharedSecret(peerPublicKey);
    std::string sessionKey = EllipticCurveCryptography::deriveSessionKey(sharedSecret);
    sessionKeys[peerId] = sessionKey;
    return true;
}

std::string CryptoManager::getPublicKeyString() const
{
    return std::to_string(ecc->getPublic());
}

uint64_t CryptoManager::getPublicKey() const
{
    return ecc->getPublic();
}

std::string CryptoManager::getQuantumRoot() const
{
    return qrs->getRoot();
}

std::string CryptoManager::quantumSign(const std::string &msg, int leafIdx)
{
    return qrs->sign(msg, leafIdx);
}

bool CryptoManager::quantumVerify(const std::string &msg, const std::string &sig, int leafIdx)
{
    return qrs->verify(msg, sig, leafIdx);
}
