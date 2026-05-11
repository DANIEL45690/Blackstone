#include "intrusion_detection.h"
#include "../core/random.h"

IntrusionDetectionSystem::IntrusionDetectionSystem() {}

void IntrusionDetectionSystem::addAlert(const std::string &severity, const std::string &type, const std::string &source, const std::string &details)
{
    UltraFastRandom rng;
    IntrusionAlert alert;
    alert.alertId = sha1(severity + type + source + std::to_string(getTimestampMs()) + rng.bytes(16));
    alert.severity = severity;
    alert.type = type;
    alert.source = source;
    alert.details = details;
    alert.timestamp = getTimestampMs();
    alert.acknowledged = false;

    alerts.push_back(alert);
    if (alerts.size() > 10000)
    {
        alerts.pop_front();
    }
}

bool IntrusionDetectionSystem::isRateLimited(const std::string &key, size_t maxAttempts, uint64_t windowMs)
{
    uint64_t now = getTimestampMs();
    auto it = rateLimits.find(key);
    if (it == rateLimits.end())
    {
        rateLimits[key] = {1, now};
        return false;
    }

    if (now - it->second.windowStart > windowMs)
    {
        it->second = {1, now};
        return false;
    }

    it->second.count++;
    return it->second.count > maxAttempts;
}

bool IntrusionDetectionSystem::checkLoginAttempt(const std::string &userId, const std::string &ip, bool success)
{
    if (!success)
    {
        std::string key = "login:" + userId + ":" + ip;
        if (isRateLimited(key, 5, 60000))
        {
            addAlert("HIGH", "BRUTE_FORCE", ip, "Multiple failed login attempts for user " + userId);
            banIp(ip, 300000);
            return false;
        }

        failedAttempts[key]++;
        if (failedAttempts[key] > 10)
        {
            banIp(ip, 3600000);
        }
    }
    else
    {
        std::string key = "login:" + userId + ":" + ip;
        failedAttempts.erase(key);
    }
    return true;
}

bool IntrusionDetectionSystem::checkRequestRate(const std::string &ip, size_t maxRequests, uint64_t windowMs)
{
    std::string key = "rate:" + ip;
    if (isRateLimited(key, maxRequests, windowMs))
    {
        addAlert("MEDIUM", "RATE_LIMIT", ip, "Exceeded request rate limit");
        return false;
    }
    return true;
}

bool IntrusionDetectionSystem::checkPacketAnomaly(const std::string &data)
{
    if (data.size() > 1000000)
    {
        addAlert("HIGH", "PACKET_SIZE", "", "Oversized packet detected: " + std::to_string(data.size()) + " bytes");
        return false;
    }

    size_t nullCount = 0;
    for (char c : data)
    {
        if (c == 0)
            nullCount++;
    }
    if (nullCount > data.size() / 2)
    {
        addAlert("MEDIUM", "NULL_FLOOD", "", "Excessive null bytes in packet");
        return false;
    }

    return true;
}

bool IntrusionDetectionSystem::isIpBanned(const std::string &ip) const
{
    auto it = banExpiry.find(ip);
    if (it == banExpiry.end())
        return false;
    return getTimestampMs() < it->second;
}

void IntrusionDetectionSystem::banIp(const std::string &ip, uint64_t durationMs)
{
    banExpiry[ip] = getTimestampMs() + durationMs;
    addAlert("HIGH", "IP_BANNED", ip, "IP address banned for " + std::to_string(durationMs) + "ms");
}

void IntrusionDetectionSystem::unbanIp(const std::string &ip)
{
    banExpiry.erase(ip);
}

std::vector<IntrusionAlert> IntrusionDetectionSystem::getRecentAlerts(size_t count) const
{
    std::lock_guard<std::mutex> lock(idsMutex);
    std::vector<IntrusionAlert> result;
    auto start = alerts.size() > count ? alerts.end() - count : alerts.begin();
    result.assign(start, alerts.end());
    return result;
}

std::vector<IntrusionAlert> IntrusionDetectionSystem::getUnacknowledgedAlerts() const
{
    std::lock_guard<std::mutex> lock(idsMutex);
    std::vector<IntrusionAlert> result;
    for (const auto &alert : alerts)
    {
        if (!alert.acknowledged)
        {
            result.push_back(alert);
        }
    }
    return result;
}

void IntrusionDetectionSystem::acknowledgeAlert(const std::string &alertId)
{
    std::lock_guard<std::mutex> lock(idsMutex);
    for (auto &alert : alerts)
    {
        if (alert.alertId == alertId)
        {
            alert.acknowledged = true;
            break;
        }
    }
}

void IntrusionDetectionSystem::clearOldAlerts(uint64_t maxAgeMs)
{
    std::lock_guard<std::mutex> lock(idsMutex);
    uint64_t now = getTimestampMs();
    while (!alerts.empty() && now - alerts.front().timestamp > maxAgeMs)
    {
        alerts.pop_front();
    }
}
