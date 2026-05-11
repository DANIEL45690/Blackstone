#include "websocket.h"
#include "../core/sha1.h"
#include <iostream>
#include <cstring>

WebSocketServer::WebSocketServer() : listenSocket(INVALID_SOCKET_T), running(false), port(0) {}

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start(int p)
{
    port = p;
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

    running = true;
    workerThreads.emplace_back([this]()
                               {
        while(running) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            socket_t client = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
            if(client != INVALID_SOCKET_T) {
                workerThreads.emplace_back([this, client]() { handleClient(client); });
            }
        } });

    return true;
}

void WebSocketServer::stop()
{
    running = false;
    if (listenSocket != INVALID_SOCKET_T)
    {
        close_socket(listenSocket);
        listenSocket = INVALID_SOCKET_T;
    }
    for (auto &t : workerThreads)
    {
        if (t.joinable())
            t.join();
    }
    workerThreads.clear();
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto &[sock, id] : clients)
    {
        close_socket(sock);
    }
    clients.clear();
    clientSockets.clear();
}

void WebSocketServer::handleClient(socket_t clientSocket)
{
    char buffer[4096];
    int n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0)
    {
        close_socket(clientSocket);
        return;
    }
    buffer[n] = 0;

    std::string request(buffer);
    size_t keyPos = request.find("Sec-WebSocket-Key:");
    if (keyPos == std::string::npos)
    {
        close_socket(clientSocket);
        return;
    }

    std::string key = request.substr(keyPos + 18);
    key = key.substr(0, key.find_first_of("\r\n"));
    std::string acceptKey = generateHandshakeKey(key);

    std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: " +
                           acceptKey + "\r\n\r\n";
    send(clientSocket, response.c_str(), response.size(), 0);

    std::string clientId = sha1(std::to_string(getTimestampMs()) + std::to_string(clientSocket));
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients[clientSocket] = clientId;
        clientSockets[clientId] = clientSocket;
    }

    if (connectHandler)
        connectHandler(clientId);

    bool closed = false;
    while (running && !closed)
    {
        std::string msg = receiveFrame(clientSocket, closed);
        if (!msg.empty() && messageHandler)
        {
            messageHandler(clientId, msg);
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(clientSocket);
        clientSockets.erase(clientId);
        for (auto it = clientTopics.begin(); it != clientTopics.end();)
        {
            if (it->first == clientId)
                it = clientTopics.erase(it);
            else
                ++it;
        }
    }

    if (disconnectHandler)
        disconnectHandler(clientId);
    close_socket(clientSocket);
}

void WebSocketServer::sendFrame(socket_t sock, const std::string &data)
{
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    size_t len = data.size();
    if (len <= 125)
    {
        frame.push_back(0x80 | len);
    }
    else if (len <= 65535)
    {
        frame.push_back(0x80 | 126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    }
    else
    {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; i--)
        {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    for (char c : data)
        frame.push_back(c);
    send(sock, (char *)frame.data(), frame.size(), 0);
}

std::string WebSocketServer::receiveFrame(socket_t sock, bool &closed)
{
    uint8_t header[2];
    int n = recv(sock, (char *)header, 2, 0);
    if (n != 2)
    {
        closed = true;
        return "";
    }

    bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    if (payloadLen == 126)
    {
        uint8_t ext[2];
        recv(sock, (char *)ext, 2, 0);
        payloadLen = (ext[0] << 8) | ext[1];
    }
    else if (payloadLen == 127)
    {
        uint8_t ext[8];
        recv(sock, (char *)ext, 8, 0);
        payloadLen = 0;
        for (int i = 0; i < 8; i++)
            payloadLen = (payloadLen << 8) | ext[i];
    }

    uint8_t mask[4];
    if (masked)
    {
        recv(sock, (char *)mask, 4, 0);
    }

    std::vector<uint8_t> payload(payloadLen);
    size_t received = 0;
    while (received < payloadLen)
    {
        int r = recv(sock, (char *)payload.data() + received, payloadLen - received, 0);
        if (r <= 0)
        {
            closed = true;
            return "";
        }
        received += r;
    }

    if (masked)
    {
        for (uint64_t i = 0; i < payloadLen; i++)
        {
            payload[i] ^= mask[i % 4];
        }
    }

    if (opcode == 0x08)
    {
        closed = true;
        return "";
    }

    return std::string(payload.begin(), payload.end());
}

std::string WebSocketServer::generateHandshakeKey(const std::string &key)
{
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    return sha1(key + magic);
}

void WebSocketServer::broadcast(const std::string &message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto &[sock, id] : clients)
    {
        sendFrame(sock, message);
    }
}

void WebSocketServer::sendTo(const std::string &clientId, const std::string &message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clientSockets.find(clientId);
    if (it != clientSockets.end())
    {
        sendFrame(it->second, message);
    }
}

void WebSocketServer::sendToTopic(const std::string &topic, const std::string &message)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto &[clientId, clientTopic] : clientTopics)
    {
        if (clientTopic == topic)
        {
            auto it = clientSockets.find(clientId);
            if (it != clientSockets.end())
            {
                sendFrame(it->second, message);
            }
        }
    }
}

void WebSocketServer::subscribe(const std::string &clientId, const std::string &topic)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    clientTopics[clientId] = topic;
}

void WebSocketServer::unsubscribe(const std::string &clientId, const std::string &topic)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = clientTopics.find(clientId);
    if (it != clientTopics.end() && it->second == topic)
    {
        clientTopics.erase(it);
    }
}

void WebSocketServer::onMessage(MessageHandler handler)
{
    messageHandler = handler;
}

void WebSocketServer::onConnect(ConnectionHandler handler)
{
    connectHandler = handler;
}

void WebSocketServer::onDisconnect(ConnectionHandler handler)
{
    disconnectHandler = handler;
}

std::vector<std::string> WebSocketServer::getConnectedClients() const
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::vector<std::string> result;
    for (const auto &[sock, id] : clients)
    {
        result.push_back(id);
    }
    return result;
}

size_t WebSocketServer::getClientCount() const
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    return clients.size();
}

bool WebSocketServer::isRunning() const
{
    return running;
}

WebSocketClient::WebSocketClient() : sock(INVALID_SOCKET_T), port(0), connected(false), running(false) {}

WebSocketClient::~WebSocketClient()
{
    disconnect();
}

bool WebSocketClient::connect(const std::string &address, int p)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_T)
        return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(p);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR_T)
    {
        close_socket(sock);
        return false;
    }

    std::string key = sha1(std::to_string(getTimestampMs()));
    std::string request = "GET / HTTP/1.1\r\n"
                          "Host: " +
                          address + "\r\n"
                                    "Upgrade: websocket\r\n"
                                    "Connection: Upgrade\r\n"
                                    "Sec-WebSocket-Key: " +
                          key + "\r\n"
                                "Sec-WebSocket-Version: 13\r\n\r\n";

    send(sock, request.c_str(), request.size(), 0);

    char buffer[4096];
    int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0)
    {
        close_socket(sock);
        return false;
    }
    buffer[n] = 0;

    std::string response(buffer);
    if (response.find("101") == std::string::npos)
    {
        close_socket(sock);
        return false;
    }

    connected = true;
    running = true;
    receiveThread = std::thread(&WebSocketClient::receiveLoop, this);

    return true;
}

void WebSocketClient::disconnect()
{
    running = false;
    connected = false;
    if (receiveThread.joinable())
        receiveThread.join();
    if (sock != INVALID_SOCKET_T)
    {
        close_socket(sock);
        sock = INVALID_SOCKET_T;
    }
}

void WebSocketClient::send(const std::string &message)
{
    if (!connected)
        return;
    sendFrame(message);
}

void WebSocketClient::sendBinary(const std::vector<uint8_t> &data)
{
    if (!connected)
        return;
    std::string str(data.begin(), data.end());
    sendFrame(str);
}

void WebSocketClient::sendFrame(const std::string &data)
{
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    size_t len = data.size();
    if (len <= 125)
    {
        frame.push_back(len);
    }
    else if (len <= 65535)
    {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--)
        {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    for (char c : data)
        frame.push_back(c);
    send(sock, (char *)frame.data(), frame.size(), 0);
}

std::string WebSocketClient::receiveFrame(bool &closed)
{
    uint8_t header[2];
    int n = recv(sock, (char *)header, 2, 0);
    if (n != 2)
    {
        closed = true;
        return "";
    }

    bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    if (payloadLen == 126)
    {
        uint8_t ext[2];
        recv(sock, (char *)ext, 2, 0);
        payloadLen = (ext[0] << 8) | ext[1];
    }
    else if (payloadLen == 127)
    {
        uint8_t ext[8];
        recv(sock, (char *)ext, 8, 0);
        payloadLen = 0;
        for (int i = 0; i < 8; i++)
            payloadLen = (payloadLen << 8) | ext[i];
    }

    std::vector<uint8_t> payload(payloadLen);
    size_t received = 0;
    while (received < payloadLen)
    {
        int r = recv(sock, (char *)payload.data() + received, payloadLen - received, 0);
        if (r <= 0)
        {
            closed = true;
            return "";
        }
        received += r;
    }

    if (opcode == 0x08)
    {
        closed = true;
        return "";
    }

    return std::string(payload.begin(), payload.end());
}

void WebSocketClient::receiveLoop()
{
    while (running && connected)
    {
        bool closed = false;
        std::string msg = receiveFrame(closed);
        if (closed)
        {
            connected = false;
            break;
        }
    }
}

bool WebSocketClient::isConnected() const
{
    return connected;
}
