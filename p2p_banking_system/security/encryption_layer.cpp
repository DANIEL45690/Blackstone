#include "encryption_layer.h"

EncryptionLayer::EncryptionLayer(const std::string &id) : nodeId(id) {}

bool EncryptionLayer::initialize(const std::string &masterKey)
{
    aes.setKey(masterKey);
    ecc = EllipticCurveCryptography();
    return aes.isInitialized();
}

std::string EncryptionLayer::encryptForPeer(const std::string &peerId, const std::string &plaintext)
{
    std::lock_guard<std::mutex> lock(layerMutex);
    auto it = sessionKeys.find(peerId);
    if (it == sessionKeys.end())
        return "";
    AdvancedAES256 peerAes(it->second);
    return peerAes.encryptGCM(plaintext);
}

std::string EncryptionLayer::decryptFromPeer(const std::string &peerId, const std::string &ciphertext)
{
    std::lock_guard<std::mutex> lock(layerMutex);
    auto it = sessionKeys.find(peerId);
    if (it == sessionKeys.end())
        return "";
    AdvancedAES256 peerAes(it->second);
    return peerAes.decryptGCM(ciphertext);
}

bool EncryptionLayer::establishPeerSession(const std::string &peerId, uint64_t peerPublicKey)
{
    std::lock_guard<std::mutex> lock(layerMutex);
    uint64_t sharedSecret = ecc.computeSharedSecret(peerPublicKey);
    std::string sessionKey = EllipticCurveCryptography::deriveSessionKey(sharedSecret);
    sessionKeys[peerId] = sessionKey;
    return true;
}

std::string EncryptionLayer::getPublicKeyString() const
{
    return std::to_string(ecc.getPublic());
}

uint64_t EncryptionLayer::getPublicKey() const
{
    return ecc.getPublic();
}

std::string EncryptionLayer::signData(const std::string &data)
{
    auto sig = ecc.sign(data);
    std::stringstream ss;
    ss << std::hex << sig.first << ":" << std::hex << sig.second;
    return ss.str();
}

bool EncryptionLayer::verifySignature(const std::string &data, const std::string &signature, uint64_t publicKey)
{
    size_t colon = signature.find(':');
    if (colon == std::string::npos)
        return false;
    uint64_t r1 = std::stoull(signature.substr(0, colon), nullptr, 16);
    uint64_t s = std::stoull(signature.substr(colon + 1), nullptr, 16);
    return ecc.verify(data, r1, s, publicKey);
}

KeyRotator::KeyRotator(uint64_t intervalMs) : lastRotation(getTimestampMs()), rotationInterval(intervalMs)
{
    currentKey = sha1_double(std::to_string(getTimestampMs()));
}

std::string KeyRotator::getCurrentKey() const
{
    return currentKey;
}

std::string KeyRotator::getPreviousKey() const
{
    return previousKey;
}

void KeyRotator::rotateKey()
{
    std::lock_guard<std::mutex> lock(rotatorMutex);
    previousKey = currentKey;
    currentKey = sha1_double(currentKey + std::to_string(getTimestampMs()));
    lastRotation = getTimestampMs();
}

bool KeyRotator::shouldRotate() const
{
    return getTimestampMs() - lastRotation >= rotationInterval;
}

uint64_t KeyRotator::timeUntilRotation() const
{
    uint64_t elapsed = getTimestampMs() - lastRotation;
    if (elapsed >= rotationInterval)
        return 0;
    return rotationInterval - elapsed;
}
