#include "thread_pool.h"

ThreadPool::ThreadPool(size_t threads) : stop(false), activeTasks(0)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back([this]
                             {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    if(this->stop && this->tasks.empty()) return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                activeTasks++;
                task();
                activeTasks--;
            } });
    }
}

ThreadPool::~ThreadPool()
{
    stop = true;
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

template <class F, class... Args>
void ThreadPool::enqueue(F &&f, Args &&...args)
{
    if (stop)
        return;
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
    condition.notify_one();
}

template void ThreadPool::enqueue<std::function<void()>, std::function<void()>>(std::function<void()> &&, std::function<void()> &&);

void ThreadPool::waitForAll()
{
    while (activeTasks > 0 || !tasks.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

size_t ThreadPool::getPendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}

size_t ThreadPool::getActiveTaskCount() const
{
    return activeTasks;
}

size_t ThreadPool::getTotalThreadCount() const
{
    return workers.size();
}

void ThreadPool::resize(size_t newSize)
{
    size_t current = workers.size();
    if (newSize > current)
    {
        for (size_t i = current; i < newSize; ++i)
        {
            workers.emplace_back([this]
                                 {
                while(true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        if(stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    activeTasks++;
                    task();
                    activeTasks--;
                } });
        }
    }
    else if (newSize < current)
    {
        for (size_t i = newSize; i < current; ++i)
        {
            workers[i].detach();
        }
        workers.resize(newSize);
    }
}

void ThreadPool::clear()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!tasks.empty())
        tasks.pop();
}
