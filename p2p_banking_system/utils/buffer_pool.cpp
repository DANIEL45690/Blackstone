#include "buffer_pool.h"
#include <cstring>

BufferPool::BufferPool(size_t bufferSizeBytes, size_t maxBufferCount)
    : bufferSize(bufferSizeBytes), maxBuffers(maxBufferCount)
{
    buffers.reserve(maxBuffers);
    for (size_t i = 0; i < maxBuffers; i++)
    {
        Buffer buf;
        buf.data.resize(bufferSizeBytes);
        buf.size = bufferSizeBytes;
        buf.inUse = false;
        buffers.push_back(buf);
        available.push(i);
    }
}

BufferPool::~BufferPool() {}

uint8_t *BufferPool::acquire()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    if (available.empty())
        return nullptr;
    size_t idx = available.front();
    available.pop();
    buffers[idx].inUse = true;
    return buffers[idx].data.data();
}

void BufferPool::release(uint8_t *ptr)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    for (size_t i = 0; i < buffers.size(); i++)
    {
        if (buffers[i].inUse && buffers[i].data.data() == ptr)
        {
            buffers[i].inUse = false;
            available.push(i);
            break;
        }
    }
}

size_t BufferPool::getBufferSize() const
{
    return bufferSize;
}

size_t BufferPool::getAvailableCount() const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    return available.size();
}

size_t BufferPool::getUsedCount() const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    size_t used = 0;
    for (const auto &buf : buffers)
    {
        if (buf.inUse)
            used++;
    }
    return used;
}

void BufferPool::resize(size_t newBufferSize, size_t newMaxBuffers)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    bufferSize = newBufferSize;
    maxBuffers = newMaxBuffers;

    buffers.clear();
    while (!available.empty())
        available.pop();

    buffers.reserve(maxBuffers);
    for (size_t i = 0; i < maxBuffers; i++)
    {
        Buffer buf;
        buf.data.resize(bufferSize);
        buf.size = bufferSize;
        buf.inUse = false;
        buffers.push_back(buf);
        available.push(i);
    }
}

void BufferPool::clear()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    for (auto &buf : buffers)
    {
        buf.inUse = false;
        std::fill(buf.data.begin(), buf.data.end(), 0);
    }
    while (!available.empty())
        available.pop();
    for (size_t i = 0; i < buffers.size(); i++)
    {
        available.push(i);
    }
}

MemoryPool::MemoryPool(size_t blockSizeBytes, size_t blockCount)
    : blockSize(blockSizeBytes), blockCount(blockCount)
{
    pools.resize(blockCount);
    used.resize(blockCount, false);
    for (size_t i = 0; i < blockCount; i++)
    {
        pools[i].resize(blockSizeBytes, 0);
    }
}

MemoryPool::~MemoryPool() {}

void *MemoryPool::allocate()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    for (size_t i = 0; i < blockCount; i++)
    {
        if (!used[i])
        {
            used[i] = true;
            return pools[i].data();
        }
    }
    return nullptr;
}

void MemoryPool::deallocate(void *ptr)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    for (size_t i = 0; i < blockCount; i++)
    {
        if (pools[i].data() == ptr)
        {
            used[i] = false;
            std::fill(pools[i].begin(), pools[i].end(), 0);
            break;
        }
    }
}

size_t MemoryPool::getBlockSize() const
{
    return blockSize;
}

size_t MemoryPool::getFreeCount() const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    size_t freeCount = 0;
    for (bool u : used)
    {
        if (!u)
            freeCount++;
    }
    return freeCount;
}

size_t MemoryPool::getUsedCount() const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    size_t usedCount = 0;
    for (bool u : used)
    {
        if (u)
            usedCount++;
    }
    return usedCount;
}
