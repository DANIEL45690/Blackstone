#ifndef HEALTH_CHECK_H
#define HEALTH_CHECK_H

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <mutex>

struct HealthStatus
{
    bool healthy;
    std::string component;
    std::string message;
    uint64_t lastCheck;
    uint64_t responseTimeMs;
};

class HealthChecker
{
    std::map<std::string, std::function<HealthStatus()>> checks;
    std::map<std::string, HealthStatus> statuses;
    std::mutex healthMutex;
    bool running;
    std::thread worker;

    void loop();

public:
    HealthChecker();
    ~HealthChecker();

    void registerCheck(const std::string &name, std::function<HealthStatus()> check);
    void unregisterCheck(const std::string &name);
    HealthStatus getStatus(const std::string &name) const;
    std::map<std::string, HealthStatus> getAllStatuses() const;
    bool isOverallHealthy() const;
    void startMonitoring(int intervalMs = 5000);
    void stopMonitoring();
};

#endif
