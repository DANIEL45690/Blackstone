#ifndef P2P_NETWORK_H
#define P2P_NETWORK_H

#include "../core/platform.h"
#include <map>
#include <set>
#include <queue>
#include <thread>
#include <atomic>
#include <functional>

class P2PNetworkCore
{
public:
    using MessageHandler = std::function<void(const std::string &, socket_t)>;

private:
    socket_t listenSocket;
    std::map<socket_t, std::string> connectedPeers;
    std::map<std::string, socket_t> peerSockets;
    std::map<std::string, std::pair<std::string, int>> peerAddresses;
    std::mutex peersMutex;
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    std::string nodeId;
    std::string cryptoKey;
    int listenPort;
    MessageHandler messageHandler;

    void handlePeer(socket_t clientSocket);
    void processMessage(const std::string &msg, socket_t from);
    void sendTo(socket_t sock, const std::string &msg);

public:
    P2PNetworkCore(const std::string &id, const std::string &key);
    ~P2PNetworkCore();

    bool startServer(int port);
    bool connectToPeer(const std::string &peerId, const std::string &address, int port);
    void disconnectPeer(const std::string &peerId);
    void broadcast(const std::string &message);
    void sendToPeer(const std::string &peerId, const std::string &message);
    void setMessageHandler(MessageHandler handler);
    std::vector<std::string> getConnectedPeers() const;
    size_t getPeerCount() const;
    void shutdown();
    bool isRunning() const;
};

#endif
