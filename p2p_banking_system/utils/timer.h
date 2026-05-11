#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>

class Timer
{
    std::function<void()> callback;
    std::chrono::milliseconds interval;
    std::atomic<bool> running;
    std::thread worker;

public:
    Timer();
    ~Timer();

    void start(std::function<void()> cb, int intervalMs);
    void stop();
    bool isRunning() const;
    void restart();
};

class TimerManager
{
    struct TimerInfo
    {
        std::function<void()> callback;
        std::chrono::steady_clock::time_point nextRun;
        std::chrono::milliseconds interval;
        bool repeating;
        bool active;
    };

    std::map<int, TimerInfo> timers;
    std::mutex managerMutex;
    std::atomic<int> nextId;
    std::thread worker;
    std::atomic<bool> running;

    void loop();

public:
    TimerManager();
    ~TimerManager();

    int setTimeout(std::function<void()> callback, int delayMs);
    int setInterval(std::function<void()> callback, int intervalMs);
    bool clearTimer(int timerId);
    void clearAll();
    size_t getActiveTimerCount() const;
};

#endif
