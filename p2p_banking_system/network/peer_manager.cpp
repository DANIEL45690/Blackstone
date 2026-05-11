#include "peer_manager.h"
#include "../core/random.h"
#include <sstream>

PeerInfo::PeerInfo() : port(0), lastSeen(0), latency(0), score(0), isActive(false) {}

PeerInfo::PeerInfo(const std::string &id, const std::string &addr, int p)
    : peerId(id), address(addr), port(p), lastSeen(getTimestampMs()), latency(0), score(0), isActive(true) {}

PeerManager::PeerManager(size_t maxPeersCount) : maxPeers(maxPeersCount) {}

void PeerManager::addPeer(const std::string &peerId, const std::string &address, int port)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    if (bannedPeers.find(peerId) != bannedPeers.end())
        return;
    if (knownPeers.size() >= maxPeers)
    {
        for (auto it = knownPeers.begin(); it != knownPeers.end(); ++it)
        {
            if (!it->second.isActive)
            {
                knownPeers.erase(it);
                break;
            }
        }
    }
    if (knownPeers.find(peerId) == knownPeers.end())
    {
        knownPeers[peerId] = PeerInfo(peerId, address, port);
    }
    else
    {
        knownPeers[peerId].address = address;
        knownPeers[peerId].port = port;
        knownPeers[peerId].lastSeen = getTimestampMs();
        knownPeers[peerId].isActive = true;
    }
}

void PeerManager::removePeer(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    knownPeers.erase(peerId);
}

void PeerManager::updatePeerSeen(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = knownPeers.find(peerId);
    if (it != knownPeers.end())
    {
        it->second.lastSeen = getTimestampMs();
        it->second.isActive = true;
    }
}

void PeerManager::updatePeerLatency(const std::string &peerId, uint64_t latency)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = knownPeers.find(peerId);
    if (it != knownPeers.end())
    {
        it->second.latency = (it->second.latency + latency) / 2;
    }
}

void PeerManager::adjustPeerScore(const std::string &peerId, int delta)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = knownPeers.find(peerId);
    if (it != knownPeers.end())
    {
        it->second.score += delta;
        if (it->second.score < -100)
        {
            banPeer(peerId, "Low score threshold exceeded");
        }
    }
}

void PeerManager::banPeer(const std::string &peerId, const std::string &reason)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    bannedPeers.insert(peerId);
    knownPeers.erase(peerId);
}

void PeerManager::unbanPeer(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    bannedPeers.erase(peerId);
}

bool PeerManager::isBanned(const std::string &peerId) const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    return bannedPeers.find(peerId) != bannedPeers.end();
}

PeerInfo PeerManager::getPeer(const std::string &peerId) const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = knownPeers.find(peerId);
    if (it != knownPeers.end())
        return it->second;
    return PeerInfo();
}

std::vector<PeerInfo> PeerManager::getActivePeers() const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<PeerInfo> active;
    for (const auto &[id, info] : knownPeers)
    {
        if (info.isActive)
            active.push_back(info);
    }
    return active;
}

std::vector<PeerInfo> PeerManager::getBestPeers(size_t count) const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<PeerInfo> sorted;
    for (const auto &[id, info] : knownPeers)
    {
        if (info.isActive)
            sorted.push_back(info);
    }
    std::sort(sorted.begin(), sorted.end(), [](const PeerInfo &a, const PeerInfo &b)
              {
        if(a.score != b.score) return a.score > b.score;
        return a.latency < b.latency; });
    if (sorted.size() > count)
        sorted.resize(count);
    return sorted;
}

size_t PeerManager::getPeerCount() const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    return knownPeers.size();
}

void PeerManager::cleanupInactivePeers(uint64_t maxInactiveMs)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    uint64_t now = getTimestampMs();
    for (auto it = knownPeers.begin(); it != knownPeers.end();)
    {
        if (now - it->second.lastSeen > maxInactiveMs)
        {
            it = knownPeers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::string PeerManager::serialize() const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    std::stringstream ss;
    for (const auto &[id, info] : knownPeers)
    {
        ss << id << ":" << info.address << ":" << info.port << ":" << info.lastSeen << ":" << info.score << "|";
    }
    return ss.str();
}

void PeerManager::deserialize(const std::string &data)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    std::stringstream ss(data);
    std::string entry;
    while (std::getline(ss, entry, '|'))
    {
        if (entry.empty())
            continue;
        std::stringstream ess(entry);
        std::string id, addr, portStr, lastSeenStr, scoreStr;
        std::getline(ess, id, ':');
        std::getline(ess, addr, ':');
        std::getline(ess, portStr, ':');
        std::getline(ess, lastSeenStr, ':');
        std::getline(ess, scoreStr, ':');
        if (!id.empty() && !addr.empty())
        {
            PeerInfo info(id, addr, std::stoi(portStr));
            info.lastSeen = std::stoull(lastSeenStr);
            info.score = std::stoi(scoreStr);
            knownPeers[id] = info;
        }
    }
}
