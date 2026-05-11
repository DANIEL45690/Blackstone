#include "metrics.h"

void MetricsCollector::incrementCounter(const std::string &name, uint64_t delta)
{
    counters[name].fetch_add(delta, std::memory_order_relaxed);
}

uint64_t MetricsCollector::getCounter(const std::string &name) const
{
    auto it = counters.find(name);
    if (it != counters.end())
        return it->second.load();
    return 0;
}

void MetricsCollector::setGauge(const std::string &name, double value)
{
    gauges[name].store(value, std::memory_order_relaxed);
}

double MetricsCollector::getGauge(const std::string &name) const
{
    auto it = gauges.find(name);
    if (it != gauges.end())
        return it->second.load();
    return 0;
}

void MetricsCollector::recordHistogram(const std::string &name, uint64_t value)
{
    std::lock_guard<std::mutex> lock(metricsMutex);
    histograms[name].push_back(value);
    if (histograms[name].size() > 10000)
    {
        histograms[name].erase(histograms[name].begin());
    }
}

std::vector<uint64_t> MetricsCollector::getHistogram(const std::string &name) const
{
    std::lock_guard<std::mutex> lock(metricsMutex);
    auto it = histograms.find(name);
    if (it != histograms.end())
        return it->second;
    return {};
}

void MetricsCollector::reset()
{
    counters.clear();
    gauges.clear();
    histograms.clear();
}

std::map<std::string, uint64_t> MetricsCollector::getAllCounters() const
{
    std::map<std::string, uint64_t> result;
    for (const auto &[name, counter] : counters)
    {
        result[name] = counter.load();
    }
    return result;
}

std::map<std::string, double> MetricsCollector::getAllGauges() const
{
    std::map<std::string, double> result;
    for (const auto &[name, gauge] : gauges)
    {
        result[name] = gauge.load();
    }
    return result;
}

MetricsTimer::MetricsTimer(const std::string &metricName, MetricsCollector &metrics)
    : name(metricName), collector(&metrics), startTime(std::chrono::steady_clock::now()) {}

MetricsTimer::~MetricsTimer()
{
    stop();
}

void MetricsTimer::stop()
{
    if (collector)
    {
        uint64_t elapsed = elapsedMs();
        collector->recordHistogram(name, elapsed);
        collector = nullptr;
    }
}

uint64_t MetricsTimer::elapsedMs() const
{
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count();
}
