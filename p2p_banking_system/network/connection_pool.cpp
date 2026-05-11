#include "connection_pool.h"

ConnectionPool::ConnectionPool(size_t maxConn) : maxConnections(maxConn), activeCount(0) {}

bool ConnectionPool::addConnection(const std::string &peerId, socket_t sock)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    if (connections.size() >= maxConnections)
        return false;

    Connection conn;
    conn.socket = sock;
    conn.peerId = peerId;
    conn.createdAt = getTimestampMs();
    conn.lastUsed = conn.createdAt;
    conn.isActive = true;

    connections[peerId] = conn;
    availableConnections.push(peerId);
    activeCount++;
    return true;
}

bool ConnectionPool::removeConnection(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    auto it = connections.find(peerId);
    if (it == connections.end())
        return false;

    close_socket(it->second.socket);
    connections.erase(it);
    activeCount--;
    return true;
}

socket_t ConnectionPool::getConnection(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    auto it = connections.find(peerId);
    if (it != connections.end())
    {
        it->second.lastUsed = getTimestampMs();
        return it->second.socket;
    }
    return INVALID_SOCKET_T;
}

socket_t ConnectionPool::acquireConnection(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    auto it = connections.find(peerId);
    if (it != connections.end() && it->second.isActive)
    {
        it->second.lastUsed = getTimestampMs();
        return it->second.socket;
    }
    return INVALID_SOCKET_T;
}

void ConnectionPool::releaseConnection(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    auto it = connections.find(peerId);
    if (it != connections.end())
    {
        it->second.isActive = true;
        it->second.lastUsed = getTimestampMs();
        availableConnections.push(peerId);
    }
}

bool ConnectionPool::hasConnection(const std::string &peerId) const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    return connections.find(peerId) != connections.end();
}

size_t ConnectionPool::size() const
{
    std::lock_guard<std::mutex> lock(poolMutex);
    return connections.size();
}

size_t ConnectionPool::getActiveCount() const
{
    return activeCount;
}

void ConnectionPool::cleanupIdleConnections(uint64_t idleTimeoutMs)
{
    std::lock_guard<std::mutex> lock(poolMutex);
    uint64_t now = getTimestampMs();
    for (auto it = connections.begin(); it != connections.end();)
    {
        if (now - it->second.lastUsed > idleTimeoutMs)
        {
            close_socket(it->second.socket);
            it = connections.erase(it);
            activeCount--;
        }
        else
        {
            ++it;
        }
    }
}

void ConnectionPool::closeAll()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    for (auto &[id, conn] : connections)
    {
        close_socket(conn.socket);
    }
    connections.clear();
    while (!availableConnections.empty())
        availableConnections.pop();
    activeCount = 0;
}

ConnectionLimiter::ConnectionLimiter(size_t maxPerIpConnections, size_t maxTotalConnections)
    : maxPerIp(maxPerIpConnections), maxTotal(maxTotalConnections) {}

bool ConnectionLimiter::allowConnection(const std::string &ipAddress)
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    size_t total = 0;
    for (const auto &[ip, count] : connectionCounts)
    {
        total += count;
    }
    if (total >= maxTotal)
        return false;

    size_t ipCount = connectionCounts[ipAddress];
    if (ipCount >= maxPerIp)
        return false;

    return true;
}

void ConnectionLimiter::connectionEstablished(const std::string &ipAddress)
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    connectionCounts[ipAddress]++;
}

void ConnectionLimiter::connectionClosed(const std::string &ipAddress)
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    auto it = connectionCounts.find(ipAddress);
    if (it != connectionCounts.end())
    {
        if (it->second > 1)
        {
            it->second--;
        }
        else
        {
            connectionCounts.erase(it);
        }
    }
}

size_t ConnectionLimiter::getCountForIp(const std::string &ipAddress) const
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    auto it = connectionCounts.find(ipAddress);
    if (it != connectionCounts.end())
        return it->second;
    return 0;
}

size_t ConnectionLimiter::getTotalCount() const
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    size_t total = 0;
    for (const auto &[ip, count] : connectionCounts)
    {
        total += count;
    }
    return total;
}

void ConnectionLimiter::reset()
{
    std::lock_guard<std::mutex> lock(limiterMutex);
    connectionCounts.clear();
}
