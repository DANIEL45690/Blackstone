#include "timer.h"

Timer::Timer() : running(false) {}

Timer::~Timer()
{
    stop();
}

void Timer::start(std::function<void()> cb, int intervalMs)
{
    if (running)
        stop();
    callback = cb;
    interval = std::chrono::milliseconds(intervalMs);
    running = true;
    worker = std::thread([this]()
                         {
        while(running) {
            std::this_thread::sleep_for(interval);
            if(running && callback) {
                callback();
            }
        } });
}

void Timer::stop()
{
    running = false;
    if (worker.joinable())
        worker.join();
}

bool Timer::isRunning() const
{
    return running;
}

void Timer::restart()
{
    if (running)
    {
        stop();
        start(callback, interval.count());
    }
}

TimerManager::TimerManager() : nextId(1), running(true)
{
    worker = std::thread(&TimerManager::loop, this);
}

TimerManager::~TimerManager()
{
    running = false;
    if (worker.joinable())
        worker.join();
}

void TimerManager::loop()
{
    while (running)
    {
        auto now = std::chrono::steady_clock::now();
        std::vector<int> toExecute;
        {
            std::lock_guard<std::mutex> lock(managerMutex);
            for (auto &[id, info] : timers)
            {
                if (info.active && now >= info.nextRun)
                {
                    toExecute.push_back(id);
                }
            }
        }

        for (int id : toExecute)
        {
            std::function<void()> cb;
            bool repeating = false;
            std::chrono::milliseconds interval;
            {
                std::lock_guard<std::mutex> lock(managerMutex);
                auto it = timers.find(id);
                if (it != timers.end() && it->second.active)
                {
                    cb = it->second.callback;
                    repeating = it->second.repeating;
                    interval = it->second.interval;
                }
            }

            if (cb)
                cb();

            if (repeating)
            {
                std::lock_guard<std::mutex> lock(managerMutex);
                auto it = timers.find(id);
                if (it != timers.end() && it->second.active)
                {
                    it->second.nextRun = std::chrono::steady_clock::now() + interval;
                }
            }
            else
            {
                clearTimer(id);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int TimerManager::setTimeout(std::function<void()> callback, int delayMs)
{
    std::lock_guard<std::mutex> lock(managerMutex);
    int id = nextId++;
    TimerInfo info;
    info.callback = callback;
    info.nextRun = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs);
    info.interval = std::chrono::milliseconds(delayMs);
    info.repeating = false;
    info.active = true;
    timers[id] = info;
    return id;
}

int TimerManager::setInterval(std::function<void()> callback, int intervalMs)
{
    std::lock_guard<std::mutex> lock(managerMutex);
    int id = nextId++;
    TimerInfo info;
    info.callback = callback;
    info.nextRun = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
    info.interval = std::chrono::milliseconds(intervalMs);
    info.repeating = true;
    info.active = true;
    timers[id] = info;
    return id;
}

bool TimerManager::clearTimer(int timerId)
{
    std::lock_guard<std::mutex> lock(managerMutex);
    auto it = timers.find(timerId);
    if (it != timers.end())
    {
        it->second.active = false;
        timers.erase(it);
        return true;
    }
    return false;
}

void TimerManager::clearAll()
{
    std::lock_guard<std::mutex> lock(managerMutex);
    timers.clear();
}

size_t TimerManager::getActiveTimerCount() const
{
    std::lock_guard<std::mutex> lock(managerMutex);
    return timers.size();
}
