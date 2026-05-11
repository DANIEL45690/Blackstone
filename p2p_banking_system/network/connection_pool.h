#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "../core/platform.h"
#include <map>
#include <queue>
#include <mutex>
#include <atomic>

struct Connection
{
    socket_t socket;
    std::string peerId;
    uint64_t createdAt;
    uint64_t lastUsed;
    bool isActive;
};

class ConnectionPool
{
    std::map<std::string, Connection> connections;
    std::queue<std::string> availableConnections;
    std::mutex poolMutex;
    size_t maxConnections;
    std::atomic<size_t> activeCount;

public:
    explicit ConnectionPool(size_t maxConn = 1000);

    bool addConnection(const std::string &peerId, socket_t sock);
    bool removeConnection(const std::string &peerId);
    socket_t getConnection(const std::string &peerId);
    socket_t acquireConnection(const std::string &peerId);
    void releaseConnection(const std::string &peerId);
    bool hasConnection(const std::string &peerId) const;
    size_t size() const;
    size_t getActiveCount() const;
    void cleanupIdleConnections(uint64_t idleTimeoutMs);
    void closeAll();
};

class ConnectionLimiter
{
    std::map<std::string, size_t> connectionCounts;
    std::mutex limiterMutex;
    size_t maxPerIp;
    size_t maxTotal;

public:
    ConnectionLimiter(size_t maxPerIpConnections = 10, size_t maxTotalConnections = 1000);

    bool allowConnection(const std::string &ipAddress);
    void connectionEstablished(const std::string &ipAddress);
    void connectionClosed(const std::string &ipAddress);
    size_t getCountForIp(const std::string &ipAddress) const;
    size_t getTotalCount() const;
    void reset();
};

#endif
