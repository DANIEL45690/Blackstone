#include "network.h"
#include "logger.h"
#include "crypto.h"
#include "utils.h"
#include <windows.h>

static network_t
{
    node_t nodes[MAX_NODES];
    int node_count;
    CRITICAL_SECTION lock;
    int proxy_port;
    int control_port;
    volatile int running;
}
g_network = {0};

static int g_initialized = 0;

uint64_t get_timestamp_ms(void)
{
    return GetTickCount64();
}

void network_init(void)
{
    if (g_initialized)
        return;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOG_FATAL("WSAStartup failed: %d", WSAGetLastError());
        return;
    }

    memset(&g_network, 0, sizeof(g_network));
    InitializeCriticalSection(&g_network.lock);
    g_network.running = 1;
    g_network.proxy_port = DEFAULT_PROXY_PORT;
    g_network.control_port = DEFAULT_CONTROL_PORT;

    LOG_INFO("Network initialized: proxy=%d, control=%d", DEFAULT_PROXY_PORT, DEFAULT_CONTROL_PORT);
    g_initialized = 1;
}

void network_cleanup(void)
{
    if (!g_initialized)
        return;

    EnterCriticalSection(&g_network.lock);
    g_network.running = 0;

    for (int i = 0; i < g_network.node_count; i++)
    {
        if (g_network.nodes[i].socket != INVALID_SOCKET_VAL)
        {
            close_socket(g_network.nodes[i].socket);
            g_network.nodes[i].socket = INVALID_SOCKET_VAL;
        }
        DeleteCriticalSection(&g_network.nodes[i].lock);
    }

    g_network.node_count = 0;
    LeaveCriticalSection(&g_network.lock);

    DeleteCriticalSection(&g_network.lock);
    WSACleanup();
    g_initialized = 0;

    LOG_INFO("Network cleaned up");
}

int network_create_node(int port)
{
    node_t node = {0};
    node.port = port;
    node.socket = socket(AF_INET, SOCK_STREAM, 0);

    if (node.socket == INVALID_SOCKET_VAL)
    {
        LOG_ERROR("Failed to create socket for port %d: %d", port, WSAGetLastError());
        return -1;
    }

    int opt = 1;
    setsockopt(node.socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    node.addr.sin_family = AF_INET;
    node.addr.sin_addr.s_addr = INADDR_ANY;
    node.addr.sin_port = htons(port);

    if (bind(node.socket, (struct sockaddr *)&node.addr, sizeof(node.addr)) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to bind port %d: %d", port, WSAGetLastError());
        close_socket(node.socket);
        return -1;
    }

    if (listen(node.socket, SOMAXCONN) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to listen on port %d: %d", port, WSAGetLastError());
        close_socket(node.socket);
        return -1;
    }

    InitializeCriticalSection(&node.lock);
    node.connection_start = get_timestamp_ms();
    node.last_heartbeat = node.connection_start;
    node.last_activity = node.connection_start;
    node.socket = INVALID_SOCKET_VAL;

    EnterCriticalSection(&g_network.lock);
    int node_id = g_network.node_count;
    g_network.nodes[g_network.node_count++] = node;
    LeaveCriticalSection(&g_network.lock);

    LOG_INFO("Node created: ID=%d, port=%d", node_id, port);
    return node_id;
}

int network_connect_node(int node_id, const char *target_ip, int target_port)
{
    if (node_id < 0 || node_id >= g_network.node_count)
    {
        LOG_ERROR("Invalid node ID: %d", node_id);
        return -1;
    }

    node_t *node = &g_network.nodes[node_id];

    EnterCriticalSection(&node->lock);

    if (node->connected)
    {
        LOG_WARN("Node %d already connected", node_id);
        LeaveCriticalSection(&node->lock);
        return 0;
    }

    socket_t client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET_VAL)
    {
        LOG_ERROR("Failed to create client socket: %d", WSAGetLastError());
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    target_addr.sin_addr.s_addr = inet_addr(target_ip);

    if (connect(client_socket, (struct sockaddr *)&target_addr, sizeof(target_addr)) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to connect to %s:%d: %d", target_ip, target_port, WSAGetLastError());
        close_socket(client_socket);
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    if (node->socket != INVALID_SOCKET_VAL && node->socket != client_socket)
    {
        close_socket(node->socket);
    }

    node->socket = client_socket;
    node->connected = 1;
    strcpy(node->ip, target_ip);
    node->port = target_port;
    node->last_heartbeat = get_timestamp_ms();
    node->last_activity = node->last_heartbeat;

    LOG_INFO("Node %d connected to %s:%d", node_id, target_ip, target_port);

    LeaveCriticalSection(&node->lock);
    return 0;
}

int network_send_packet(int node_id, int type, const uint8_t *data, size_t len)
{
    if (node_id < 0 || node_id >= g_network.node_count)
        return -1;

    node_t *node = &g_network.nodes[node_id];

    EnterCriticalSection(&node->lock);

    if (!node->connected || node->socket == INVALID_SOCKET_VAL)
    {
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    uint8_t header[12];
    header[0] = (uint8_t)type;
    *(uint32_t *)(header + 1) = htonl((uint32_t)len);
    *(uint32_t *)(header + 5) = htonl(crc32(data, len));
    *(uint16_t *)(header + 9) = htons((uint16_t)(get_timestamp_ms() & 0xFFFF));

    int sent = send(node->socket, (char *)header, 12, 0);
    if (sent != 12)
    {
        node->errors++;
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    sent = send(node->socket, (char *)data, (int)len, 0);
    if (sent > 0)
    {
        node->packets_sent++;
        node->bytes_sent += sent;
        node->last_activity = get_timestamp_ms();
    }
    else
    {
        node->errors++;
    }

    LeaveCriticalSection(&node->lock);
    return sent;
}

int network_receive_packet(int node_id, uint8_t *buffer, size_t buffer_size, size_t *out_len)
{
    if (node_id < 0 || node_id >= g_network.node_count)
        return -1;

    node_t *node = &g_network.nodes[node_id];

    EnterCriticalSection(&node->lock);

    if (!node->connected || node->socket == INVALID_SOCKET_VAL)
    {
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(node->socket, &read_fds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    int select_result = select(0, &read_fds, NULL, NULL, &tv);

    if (select_result <= 0 || !FD_ISSET(node->socket, &read_fds))
    {
        LeaveCriticalSection(&node->lock);
        return 0;
    }

    uint8_t header[12];
    int received = recv(node->socket, (char *)header, 12, MSG_PEEK);

    if (received <= 0)
    {
        LeaveCriticalSection(&node->lock);
        return 0;
    }

    if (received < 12)
    {
        LeaveCriticalSection(&node->lock);
        return 0;
    }

    int type = header[0];
    uint32_t data_len = ntohl(*(uint32_t *)(header + 1));
    uint32_t expected_crc = ntohl(*(uint32_t *)(header + 5));

    if (data_len > buffer_size)
    {
        LOG_ERROR("Packet too large: %u > %zu", data_len, buffer_size);
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    received = recv(node->socket, (char *)header, 12, 0);
    if (received != 12)
    {
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    received = recv(node->socket, (char *)buffer, data_len, 0);
    if (received != (int)data_len)
    {
        LOG_ERROR("Incomplete packet: expected %u, got %d", data_len, received);
        node->errors++;
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    uint32_t actual_crc = crc32(buffer, data_len);
    if (actual_crc != expected_crc)
    {
        LOG_ERROR("CRC mismatch: expected 0x%08X, got 0x%08X", expected_crc, actual_crc);
        node->errors++;
        LeaveCriticalSection(&node->lock);
        return -1;
    }

    node->packets_received++;
    node->bytes_received += received;
    node->last_activity = get_timestamp_ms();
    *out_len = data_len;

    LeaveCriticalSection(&node->lock);
    return type;
}

int network_handshake(int node_id)
{
    uint8_t random_data[32];
    random_bytes(random_data, 32);

    if (network_send_packet(node_id, PACKET_HANDSHAKE, random_data, 32) < 0)
    {
        return 0;
    }

    uint8_t buffer[256];
    size_t out_len;

    int result = network_receive_packet(node_id, buffer, sizeof(buffer), &out_len);
    if (result != PACKET_HANDSHAKE || out_len < 32)
    {
        return 0;
    }

    node_t *node = &g_network.nodes[node_id];

    EnterCriticalSection(&node->lock);

    for (int i = 0; i < 32; i++)
    {
        node->session_key[i] = random_data[i] ^ buffer[i];
    }
    node->authenticated = 1;

    LeaveCriticalSection(&node->lock);

    LOG_INFO("Handshake completed for node %d", node_id);
    return 1;
}

void network_print_stats(void)
{
    LOG_INFO("========== NETWORK STATISTICS ==========");

    EnterCriticalSection(&g_network.lock);
    for (int i = 0; i < g_network.node_count; i++)
    {
        node_t *node = &g_network.nodes[i];
        LOG_INFO("Node %d: port=%d, connected=%d, auth=%d, pkts=%llu/%llu, bytes=%llu/%llu, err=%llu",
                 i, node->port, node->connected, node->authenticated,
                 (unsigned long long)node->packets_sent, (unsigned long long)node->packets_received,
                 (unsigned long long)node->bytes_sent, (unsigned long long)node->bytes_received,
                 (unsigned long long)node->errors);
    }
    LeaveCriticalSection(&g_network.lock);

    LOG_INFO("========================================");
}

void network_check_heartbeats(void)
{
    uint64_t now = get_timestamp_ms();

    EnterCriticalSection(&g_network.lock);
    for (int i = 0; i < g_network.node_count; i++)
    {
        node_t *node = &g_network.nodes[i];
        if (node->connected && (now - node->last_heartbeat) > HEARTBEAT_INTERVAL_MS * 3)
        {
            LOG_WARN("Node %d heartbeat timeout", i);
            node->connected = 0;
            if (node->socket != INVALID_SOCKET_VAL)
            {
                close_socket(node->socket);
                node->socket = INVALID_SOCKET_VAL;
            }
        }
    }
    LeaveCriticalSection(&g_network.lock);
}

int network_get_node_count(void)
{
    EnterCriticalSection(&g_network.lock);
    int count = g_network.node_count;
    LeaveCriticalSection(&g_network.lock);
    return count;
}

node_t *network_get_node(int node_id)
{
    if (node_id < 0 || node_id >= g_network.node_count)
        return NULL;
    return &g_network.nodes[node_id];
}
