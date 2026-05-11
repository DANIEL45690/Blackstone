#ifndef INTRUSION_DETECTION_H
#define INTRUSION_DETECTION_H

#include <string>
#include <vector>
#include <map>
#include <deque>

struct IntrusionAlert
{
    std::string alertId;
    std::string severity;
    std::string type;
    std::string source;
    std::string details;
    uint64_t timestamp;
    bool acknowledged;
};

class IntrusionDetectionSystem
{
    struct RateLimit
    {
        size_t count;
        uint64_t windowStart;
    };

    std::map<std::string, RateLimit> rateLimits;
    std::deque<IntrusionAlert> alerts;
    std::map<std::string, size_t> failedAttempts;
    std::map<std::string, uint64_t> banExpiry;
    std::mutex idsMutex;

    void addAlert(const std::string &severity, const std::string &type, const std::string &source, const std::string &details);
    bool isRateLimited(const std::string &key, size_t maxAttempts, uint64_t windowMs);

public:
    IntrusionDetectionSystem();

    bool checkLoginAttempt(const std::string &userId, const std::string &ip, bool success);
    bool checkRequestRate(const std::string &ip, size_t maxRequests, uint64_t windowMs);
    bool checkPacketAnomaly(const std::string &data);
    bool isIpBanned(const std::string &ip) const;
    void banIp(const std::string &ip, uint64_t durationMs);
    void unbanIp(const std::string &ip);
    std::vector<IntrusionAlert> getRecentAlerts(size_t count = 100) const;
    std::vector<IntrusionAlert> getUnacknowledgedAlerts() const;
    void acknowledgeAlert(const std::string &alertId);
    void clearOldAlerts(uint64_t maxAgeMs);
};

#endif
