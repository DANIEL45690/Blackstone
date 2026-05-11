#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>

class BufferPool
{
    struct Buffer
    {
        std::vector<uint8_t> data;
        size_t size;
        bool inUse;
    };

    std::vector<Buffer> buffers;
    std::queue<size_t> available;
    std::mutex poolMutex;
    size_t bufferSize;
    size_t maxBuffers;

public:
    BufferPool(size_t bufferSizeBytes = 65536, size_t maxBufferCount = 100);
    ~BufferPool();

    uint8_t *acquire();
    void release(uint8_t *ptr);
    size_t getBufferSize() const;
    size_t getAvailableCount() const;
    size_t getUsedCount() const;
    void resize(size_t newBufferSize, size_t newMaxBuffers);
    void clear();
};

class MemoryPool
{
    std::vector<std::vector<uint8_t>> pools;
    std::vector<bool> used;
    std::mutex poolMutex;
    size_t blockSize;
    size_t blockCount;

public:
    MemoryPool(size_t blockSizeBytes, size_t blockCount);
    ~MemoryPool();

    void *allocate();
    void deallocate(void *ptr);
    size_t getBlockSize() const;
    size_t getFreeCount() const;
    size_t getUsedCount() const;
};

#endif
