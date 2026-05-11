#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <set>

struct PeerInfo
{
    std::string peerId;
    std::string address;
    int port;
    uint64_t lastSeen;
    uint64_t latency;
    int score;
    bool isActive;
    std::string version;

    PeerInfo();
    PeerInfo(const std::string &id, const std::string &addr, int p);
};

class PeerManager
{
    std::map<std::string, PeerInfo> knownPeers;
    std::set<std::string> bannedPeers;
    std::mutex peersMutex;
    size_t maxPeers;

public:
    explicit PeerManager(size_t maxPeersCount = 100);

    void addPeer(const std::string &peerId, const std::string &address, int port);
    void removePeer(const std::string &peerId);
    void updatePeerSeen(const std::string &peerId);
    void updatePeerLatency(const std::string &peerId, uint64_t latency);
    void adjustPeerScore(const std::string &peerId, int delta);
    void banPeer(const std::string &peerId, const std::string &reason);
    void unbanPeer(const std::string &peerId);
    bool isBanned(const std::string &peerId) const;

    PeerInfo getPeer(const std::string &peerId) const;
    std::vector<PeerInfo> getActivePeers() const;
    std::vector<PeerInfo> getBestPeers(size_t count) const;
    size_t getPeerCount() const;
    void cleanupInactivePeers(uint64_t maxInactiveMs);

    std::string serialize() const;
    void deserialize(const std::string &data);
};

#endif
