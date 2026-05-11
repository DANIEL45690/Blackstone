#include "health_check.h"
#include "../core/platform.h"

HealthChecker::HealthChecker() : running(false) {}

HealthChecker::~HealthChecker()
{
    stopMonitoring();
}

void HealthChecker::loop()
{
    while (running)
    {
        std::map<std::string, std::function<HealthStatus()>> checksCopy;
        {
            std::lock_guard<std::mutex> lock(healthMutex);
            checksCopy = checks;
        }

        for (auto &[name, check] : checksCopy)
        {
            HealthStatus status = check();
            status.lastCheck = getTimestampMs();
            {
                std::lock_guard<std::mutex> lock(healthMutex);
                statuses[name] = status;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void HealthChecker::registerCheck(const std::string &name, std::function<HealthStatus()> check)
{
    std::lock_guard<std::mutex> lock(healthMutex);
    checks[name] = check;
}

void HealthChecker::unregisterCheck(const std::string &name)
{
    std::lock_guard<std::mutex> lock(healthMutex);
    checks.erase(name);
    statuses.erase(name);
}

HealthStatus HealthChecker::getStatus(const std::string &name) const
{
    std::lock_guard<std::mutex> lock(healthMutex);
    auto it = statuses.find(name);
    if (it != statuses.end())
        return it->second;
    HealthStatus hs;
    hs.healthy = false;
    hs.component = name;
    hs.message = "No status available";
    return hs;
}

std::map<std::string, HealthStatus> HealthChecker::getAllStatuses() const
{
    std::lock_guard<std::mutex> lock(healthMutex);
    return statuses;
}

bool HealthChecker::isOverallHealthy() const
{
    std::lock_guard<std::mutex> lock(healthMutex);
    for (const auto &[name, status] : statuses)
    {
        if (!status.healthy)
            return false;
    }
    return true;
}

void HealthChecker::startMonitoring(int intervalMs)
{
    if (running)
        return;
    running = true;
    worker = std::thread(&HealthChecker::loop, this);
}

void HealthChecker::stopMonitoring()
{
    running = false;
    if (worker.joinable())
        worker.join();
}
