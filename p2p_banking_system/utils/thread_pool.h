#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <condition_variable>
#include <atomic>

class ThreadPool
{
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<size_t> activeTasks;

public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template <class F, class... Args>
    void enqueue(F &&f, Args &&...args);

    void waitForAll();
    size_t getPendingTaskCount() const;
    size_t getActiveTaskCount() const;
    size_t getTotalThreadCount() const;
    void resize(size_t newSize);
    void clear();
};

#endif
