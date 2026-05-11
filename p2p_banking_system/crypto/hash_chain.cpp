#ifndef MULTI_FACTOR_H
#define MULTI_FACTOR_H

#include <string>
#include <vector>
#include <map>
#include <functional>

class MultiFactorAuth
{
    struct Factor
    {
        std::string type;
        std::string data;
        uint64_t timestamp;
        bool verified;
    };

    std::map<std::string, std::vector<Factor>> userFactors;
    std::map<std::string, std::string> userSecrets;
    std::mutex authMutex;

public:
    bool registerPassword(const std::string &userId, const std::string &password);
    bool verifyPassword(const std::string &userId, const std::string &password);

    bool registerTOTP(const std::string &userId, const std::string &secret);
    std::string generateTOTP(const std::string &userId);
    bool verifyTOTP(const std::string &userId, const std::string &code);

    bool registerBackupCode(const std::string &userId, const std::vector<std::string> &codes);
    bool verifyBackupCode(const std::string &userId, const std::string &code);

    bool registerBiometric(const std::string &userId, const std::string &biometricHash);
    bool verifyBiometric(const std::string &userId, const std::string &biometricData);

    bool registerHardwareToken(const std::string &userId, const std::string &tokenId);
    bool verifyHardwareToken(const std::string &userId, const std::string &challenge);

    int getRequiredFactors(const std::string &userId) const;
    bool authenticate(const std::string &userId, const std::vector<std::pair<std::string, std::string>> &factors);

private:
    std::string generateTOTPToken(const std::string &secret, uint64_t timeStep);
};

class SessionManager
{
    struct Session
    {
        std::string sessionId;
        std::string userId;
        uint64_t createdAt;
        uint64_t expiresAt;
        std::string ipAddress;
        std::string userAgent;
    };

    std::map<std::string, Session> activeSessions;
    std::mutex sessionMutex;

public:
    std::string createSession(const std::string &userId, const std::string &ip, const std::string &userAgent, uint64_t lifetimeMs);
    bool validateSession(const std::string &sessionId);
    bool invalidateSession(const std::string &sessionId);
    std::string getUserId(const std::string &sessionId) const;
    void cleanupExpiredSessions();
    size_t getActiveSessionCount() const;
};

#endif
