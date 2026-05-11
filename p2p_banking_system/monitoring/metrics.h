#ifndef METRICS_H
#define METRICS_H

#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <chrono>

class MetricsCollector
{
    std::map<std::string, std::atomic<uint64_t>> counters;
    std::map<std::string, std::atomic<double>> gauges;
    std::map<std::string, std::vector<uint64_t>> histograms;
    std::mutex metricsMutex;

public:
    void incrementCounter(const std::string &name, uint64_t delta = 1);
    uint64_t getCounter(const std::string &name) const;
    void setGauge(const std::string &name, double value);
    double getGauge(const std::string &name) const;
    void recordHistogram(const std::string &name, uint64_t value);
    std::vector<uint64_t> getHistogram(const std::string &name) const;
    void reset();
    std::map<std::string, uint64_t> getAllCounters() const;
    std::map<std::string, double> getAllGauges() const;
};

class MetricsTimer
{
    std::string name;
    MetricsCollector *collector;
    std::chrono::steady_clock::time_point startTime;

public:
    MetricsTimer(const std::string &metricName, MetricsCollector &metrics);
    ~MetricsTimer();

    void stop();
    uint64_t elapsedMs() const;
};

#endif
