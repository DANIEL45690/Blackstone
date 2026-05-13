#include "proxy_client.h"
#include "../core/logger.h"
#include "../core/network.h"

#define MAX_PROXY_CLIENTS 64

typedef struct
{
    int id;
    socket_t sock;
    char proxy_ip[16];
    int proxy_port;
    char target_ip[16];
    int target_port;
    int connected;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} proxy_client_conn_t;

static proxy_client_conn_t g_proxy_clients[MAX_PROXY_CLIENTS];
static int g_client_count = 0;
static CRITICAL_SECTION g_client_lock;
static int g_initialized = 0;

static int get_next_client_id(void)
{
    for (int i = 0; i < MAX_PROXY_CLIENTS; i++)
    {
        if (!g_proxy_clients[i].connected)
        {
            return i;
        }
    }
    return -1;
}

int proxy_client_connect(const char *proxy_ip, int proxy_port, const char *target_ip, int target_port)
{
    if (!g_initialized)
    {
        InitializeCriticalSection(&g_client_lock);
        memset(g_proxy_clients, 0, sizeof(g_proxy_clients));
        g_initialized = 1;
    }

    int client_id = get_next_client_id();
    if (client_id == -1)
    {
        LOG_ERROR("Maximum proxy clients reached");
        return -1;
    }

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VAL)
    {
        LOG_ERROR("Failed to create proxy client socket");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(proxy_port);
    addr.sin_addr.s_addr = inet_addr(proxy_ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to connect to proxy %s:%d", proxy_ip, proxy_port);
        close_socket(sock);
        return -1;
    }

    EnterCriticalSection(&g_client_lock);

    g_proxy_clients[client_id].id = client_id;
    g_proxy_clients[client_id].sock = sock;
    strcpy(g_proxy_clients[client_id].proxy_ip, proxy_ip);
    g_proxy_clients[client_id].proxy_port = proxy_port;
    strcpy(g_proxy_clients[client_id].target_ip, target_ip);
    g_proxy_clients[client_id].target_port = target_port;
    g_proxy_clients[client_id].connected = 1;
    g_proxy_clients[client_id].bytes_sent = 0;
    g_proxy_clients[client_id].bytes_received = 0;

    g_client_count++;

    LeaveCriticalSection(&g_client_lock);

    LOG_INFO("Proxy client %d connected: %s:%d -> %s:%d",
             client_id, proxy_ip, proxy_port, target_ip, target_port);

    return client_id;
}

int proxy_client_disconnect(int client_id)
{
    if (client_id < 0 || client_id >= MAX_PROXY_CLIENTS)
        return -1;

    EnterCriticalSection(&g_client_lock);

    if (g_proxy_clients[client_id].connected)
    {
        close_socket(g_proxy_clients[client_id].sock);
        g_proxy_clients[client_id].connected = 0;
        g_client_count--;
        LOG_INFO("Proxy client %d disconnected", client_id);
    }

    LeaveCriticalSection(&g_client_lock);
    return 0;
}

int proxy_client_send(int client_id, const uint8_t *data, size_t len)
{
    if (client_id < 0 || client_id >= MAX_PROXY_CLIENTS)
        return -1;

    EnterCriticalSection(&g_client_lock);

    if (!g_proxy_clients[client_id].connected)
    {
        LeaveCriticalSection(&g_client_lock);
        return -1;
    }

    int sent = send(g_proxy_clients[client_id].sock, (char *)data, (int)len, 0);
    if (sent > 0)
    {
        g_proxy_clients[client_id].bytes_sent += sent;
    }

    LeaveCriticalSection(&g_client_lock);
    return sent;
}

int proxy_client_receive(int client_id, uint8_t *buffer, size_t buffer_size)
{
    if (client_id < 0 || client_id >= MAX_PROXY_CLIENTS)
        return -1;

    EnterCriticalSection(&g_client_lock);

    if (!g_proxy_clients[client_id].connected)
    {
        LeaveCriticalSection(&g_client_lock);
        return -1;
    }

    int received = recv(g_proxy_clients[client_id].sock, (char *)buffer, (int)buffer_size, 0);
    if (received > 0)
    {
        g_proxy_clients[client_id].bytes_received += received;
    }

    LeaveCriticalSection(&g_client_lock);
    return received;
}

void proxy_client_print_all(void)
{
    EnterCriticalSection(&g_client_lock);

    LOG_INFO("========== PROXY CLIENTS ==========");
    LOG_INFO("ID | Proxy | Target | Bytes S/R");

    for (int i = 0; i < MAX_PROXY_CLIENTS; i++)
    {
        if (g_proxy_clients[i].connected)
        {
            LOG_INFO("%d | %s:%d -> %s:%d | %llu / %llu",
                     g_proxy_clients[i].id,
                     g_proxy_clients[i].proxy_ip, g_proxy_clients[i].proxy_port,
                     g_proxy_clients[i].target_ip, g_proxy_clients[i].target_port,
                     (unsigned long long)g_proxy_clients[i].bytes_sent,
                     (unsigned long long)g_proxy_clients[i].bytes_received);
        }
    }

    LOG_INFO("===================================");
    LeaveCriticalSection(&g_client_lock);
}
