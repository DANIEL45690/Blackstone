#include "multi_factor.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <sstream>
#include <iomanip>
#include <ctime>

bool MultiFactorAuth::registerPassword(const std::string &userId, const std::string &password)
{
    std::lock_guard<std::mutex> lock(authMutex);
    userSecrets[userId] = sha1_double(password);
    Factor f;
    f.type = "password";
    f.data = userSecrets[userId];
    f.timestamp = getTimestampMs();
    f.verified = false;
    userFactors[userId].push_back(f);
    return true;
}

bool MultiFactorAuth::verifyPassword(const std::string &userId, const std::string &password)
{
    std::lock_guard<std::mutex> lock(authMutex);
    auto it = userSecrets.find(userId);
    if (it == userSecrets.end())
        return false;
    return it->second == sha1_double(password);
}

bool MultiFactorAuth::registerTOTP(const std::string &userId, const std::string &secret)
{
    std::lock_guard<std::mutex> lock(authMutex);
    Factor f;
    f.type = "totp";
    f.data = secret;
    f.timestamp = getTimestampMs();
    f.verified = false;
    userFactors[userId].push_back(f);
    return true;
}

std::string MultiFactorAuth::generateTOTP(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(authMutex);
    for (auto &f : userFactors[userId])
    {
        if (f.type == "totp")
        {
            return generateTOTPToken(f.data, std::time(nullptr) / 30);
        }
    }
    return "";
}

bool MultiFactorAuth::verifyTOTP(const std::string &userId, const std::string &code)
{
    std::string expected = generateTOTP(userId);
    return expected == code;
}

bool MultiFactorAuth::registerBackupCode(const std::string &userId, const std::vector<std::string> &codes)
{
    std::lock_guard<std::mutex> lock(authMutex);
    std::string codesHash;
    for (const auto &c : codes)
        codesHash += sha1(c);
    Factor f;
    f.type = "backup";
    f.data = sha1(codesHash);
    f.timestamp = getTimestampMs();
    f.verified = false;
    userFactors[userId].push_back(f);
    return true;
}

bool MultiFactorAuth::verifyBackupCode(const std::string &userId, const std::string &code)
{
    std::lock_guard<std::mutex> lock(authMutex);
    for (auto &f : userFactors[userId])
    {
        if (f.type == "backup")
        {
            return f.data.find(sha1(code)) != std::string::npos;
        }
    }
    return false;
}

bool MultiFactorAuth::registerBiometric(const std::string &userId, const std::string &biometricHash)
{
    std::lock_guard<std::mutex> lock(authMutex);
    Factor f;
    f.type = "biometric";
    f.data = biometricHash;
    f.timestamp = getTimestampMs();
    f.verified = false;
    userFactors[userId].push_back(f);
    return true;
}

bool MultiFactorAuth::verifyBiometric(const std::string &userId, const std::string &biometricData)
{
    std::lock_guard<std::mutex> lock(authMutex);
    for (auto &f : userFactors[userId])
    {
        if (f.type == "biometric")
        {
            return f.data == sha1(biometricData);
        }
    }
    return false;
}

bool MultiFactorAuth::registerHardwareToken(const std::string &userId, const std::string &tokenId)
{
    std::lock_guard<std::mutex> lock(authMutex);
    Factor f;
    f.type = "hardware";
    f.data = sha1(tokenId);
    f.timestamp = getTimestampMs();
    f.verified = false;
    userFactors[userId].push_back(f);
    return true;
}

bool MultiFactorAuth::verifyHardwareToken(const std::string &userId, const std::string &challenge)
{
    std::lock_guard<std::mutex> lock(authMutex);
    for (auto &f : userFactors[userId])
    {
        if (f.type == "hardware")
        {
            return sha1(challenge + f.data).substr(0, 16) == challenge.substr(0, 16);
        }
    }
    return false;
}

int MultiFactorAuth::getRequiredFactors(const std::string &userId) const
{
    return 2;
}

bool MultiFactorAuth::authenticate(const std::string &userId, const std::vector<std::pair<std::string, std::string>> &factors)
{
    int verifiedCount = 0;
    for (const auto &[type, value] : factors)
    {
        if (type == "password" && verifyPassword(userId, value))
            verifiedCount++;
        if (type == "totp" && verifyTOTP(userId, value))
            verifiedCount++;
        if (type == "backup" && verifyBackupCode(userId, value))
            verifiedCount++;
        if (type == "biometric" && verifyBiometric(userId, value))
            verifiedCount++;
        if (type == "hardware" && verifyHardwareToken(userId, value))
            verifiedCount++;
    }
    return verifiedCount >= getRequiredFactors(userId);
}

std::string MultiFactorAuth::generateTOTPToken(const std::string &secret, uint64_t timeStep)
{
    std::string data = secret + std::to_string(timeStep);
    std::string hash = sha1(data);
    int offset = hash[hash.size() - 1] & 0xF;
    int code = ((hash[offset] & 0x7F) << 24) |
               ((hash[offset + 1] & 0xFF) << 16) |
               ((hash[offset + 2] & 0xFF) << 8) |
               (hash[offset + 3] & 0xFF);
    code %= 1000000;
    std::stringstream ss;
    ss << std::setw(6) << std::setfill('0') << code;
    return ss.str();
}

std::string SessionManager::createSession(const std::string &userId, const std::string &ip, const std::string &userAgent, uint64_t lifetimeMs)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    UltraFastRandom rng;
    std::string sessionId = sha1(userId + ip + std::to_string(getTimestampMs()) + rng.bytes(32));
    Session s;
    s.sessionId = sessionId;
    s.userId = userId;
    s.createdAt = getTimestampMs();
    s.expiresAt = s.createdAt + lifetimeMs;
    s.ipAddress = ip;
    s.userAgent = userAgent;
    activeSessions[sessionId] = s;
    return sessionId;
}

bool SessionManager::validateSession(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = activeSessions.find(sessionId);
    if (it == activeSessions.end())
        return false;
    if (getTimestampMs() > it->second.expiresAt)
    {
        activeSessions.erase(it);
        return false;
    }
    return true;
}

bool SessionManager::invalidateSession(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    return activeSessions.erase(sessionId) > 0;
}

std::string SessionManager::getUserId(const std::string &sessionId) const
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = activeSessions.find(sessionId);
    if (it == activeSessions.end())
        return "";
    return it->second.userId;
}

void SessionManager::cleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    uint64_t now = getTimestampMs();
    for (auto it = activeSessions.begin(); it != activeSessions.end();)
    {
        if (now > it->second.expiresAt)
        {
            it = activeSessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t SessionManager::getActiveSessionCount() const
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    return activeSessions.size();
}
