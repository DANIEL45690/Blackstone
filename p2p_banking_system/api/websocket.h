#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "../core/platform.h"
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <atomic>

class WebSocketServer
{
public:
    using MessageHandler = std::function<void(const std::string &, const std::string &)>;
    using ConnectionHandler = std::function<void(const std::string &)>;

private:
    socket_t listenSocket;
    std::map<socket_t, std::string> clients;
    std::map<std::string, socket_t> clientSockets;
    std::map<std::string, std::string> clientTopics;
    std::mutex clientsMutex;
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    int port;
    MessageHandler messageHandler;
    ConnectionHandler connectHandler;
    ConnectionHandler disconnectHandler;

    void handleClient(socket_t clientSocket);
    void sendFrame(socket_t sock, const std::string &data);
    std::string receiveFrame(socket_t sock, bool &closed);
    std::string generateHandshakeKey(const std::string &key);

public:
    WebSocketServer();
    ~WebSocketServer();

    bool start(int port);
    void stop();
    void broadcast(const std::string &message);
    void sendTo(const std::string &clientId, const std::string &message);
    void sendToTopic(const std::string &topic, const std::string &message);
    void subscribe(const std::string &clientId, const std::string &topic);
    void unsubscribe(const std::string &clientId, const std::string &topic);

    void onMessage(MessageHandler handler);
    void onConnect(ConnectionHandler handler);
    void onDisconnect(ConnectionHandler handler);

    std::vector<std::string> getConnectedClients() const;
    size_t getClientCount() const;
    bool isRunning() const;
};

class WebSocketClient
{
    socket_t sock;
    std::string serverAddress;
    int port;
    bool connected;
    std::atomic<bool> running;
    std::thread receiveThread;

    void receiveLoop();
    void sendFrame(const std::string &data);
    std::string receiveFrame(bool &closed);

public:
    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string &address, int port);
    void disconnect();
    void send(const std::string &message);
    void sendBinary(const std::vector<uint8_t> &data);
    bool isConnected() const;
};

#endif
