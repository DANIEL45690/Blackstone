#include "p2p_network.h"
#include "../core/ternary.h"
#include "../core/aes256.h"
#include "../core/sha1.h"
#include <iostream>

P2PNetworkCore::P2PNetworkCore(const std::string &id, const std::string &key)
    : listenSocket(INVALID_SOCKET_T), nodeId(id), cryptoKey(key), listenPort(0), running(true)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

P2PNetworkCore::~P2PNetworkCore()
{
    shutdown();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool P2PNetworkCore::startServer(int port)
{
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET_T)
        return false;

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenSocket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR_T)
        return false;
    if (::listen(listenSocket, 100) == SOCKET_ERROR_T)
        return false;

    listenPort = port;

    workerThreads.emplace_back([this]()
                               {
        while(running) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            socket_t client = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if(client != INVALID_SOCKET_T) {
                workerThreads.emplace_back([this, client]() { handlePeer(client); });
            }
        } });

    return true;
}

bool P2PNetworkCore::connectToPeer(const std::string &peerId, const std::string &address, int port)
{
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_T)
        return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR_T)
    {
        close_socket(sock);
        return false;
    }

    std::lock_guard<std::mutex> lock(peersMutex);
    connectedPeers[sock] = peerId;
    peerSockets[peerId] = sock;
    peerAddresses[peerId] = {address, port};
    workerThreads.emplace_back([this, sock]()
                               { handlePeer(sock); });

    return true;
}

void P2PNetworkCore::disconnectPeer(const std::string &peerId)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = peerSockets.find(peerId);
    if (it != peerSockets.end())
    {
        close_socket(it->second);
        connectedPeers.erase(it->second);
        peerSockets.erase(it);
    }
    peerAddresses.erase(peerId);
}

void P2PNetworkCore::handlePeer(socket_t clientSocket)
{
    char buffer[65536];
    while (running)
    {
        int n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;
        buffer[n] = 0;

        AdvancedAES256 aes(cryptoKey);
        std::string decoded = TernaryCodingSystem::decodeTernary(buffer);
        std::string decrypted = aes.decryptGCM(decoded);

        if (!decrypted.empty())
        {
            processMessage(decrypted, clientSocket);
        }
    }

    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = connectedPeers.find(clientSocket);
    if (it != connectedPeers.end())
    {
        std::string peerId = it->second;
        connectedPeers.erase(clientSocket);
        peerSockets.erase(peerId);
    }
    close_socket(clientSocket);
}

void P2PNetworkCore::processMessage(const std::string &msg, socket_t from)
{
    if (messageHandler)
    {
        messageHandler(msg, from);
    }
}

void P2PNetworkCore::sendTo(socket_t sock, const std::string &msg)
{
    AdvancedAES256 aes(cryptoKey);
    std::string encrypted = aes.encryptGCM(msg);
    std::string encoded = TernaryCodingSystem::encodeBinary(encrypted);
    send(sock, encoded.c_str(), (int)encoded.size(), 0);
}

void P2PNetworkCore::broadcast(const std::string &message)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    for (auto &[sock, id] : connectedPeers)
    {
        sendTo(sock, message);
    }
}

void P2PNetworkCore::sendToPeer(const std::string &peerId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = peerSockets.find(peerId);
    if (it != peerSockets.end())
    {
        sendTo(it->second, message);
    }
}

void P2PNetworkCore::setMessageHandler(MessageHandler handler)
{
    messageHandler = handler;
}

std::vector<std::string> P2PNetworkCore::getConnectedPeers() const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    std::vector<std::string> peers;
    for (const auto &[id, sock] : peerSockets)
    {
        peers.push_back(id);
    }
    return peers;
}

size_t P2PNetworkCore::getPeerCount() const
{
    std::lock_guard<std::mutex> lock(peersMutex);
    return peerSockets.size();
}

void P2PNetworkCore::shutdown()
{
    running = false;

    for (auto &t : workerThreads)
    {
        if (t.joinable())
            t.join();
    }

    if (listenSocket != INVALID_SOCKET_T)
    {
        close_socket(listenSocket);
        listenSocket = INVALID_SOCKET_T;
    }

    std::lock_guard<std::mutex> lock(peersMutex);
    for (auto &[sock, id] : connectedPeers)
    {
        close_socket(sock);
    }
    connectedPeers.clear();
    peerSockets.clear();
}

bool P2PNetworkCore::isRunning() const
{
    return running;
}
