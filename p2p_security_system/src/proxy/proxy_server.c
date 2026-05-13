#include "proxy_server.h"
#include "../core/logger.h"
#include "../core/network.h"

#define MAX_PROXY_CLIENTS 100
#define PROXY_BUFFER_SIZE 8192

static proxy_client_t g_clients[MAX_PROXY_CLIENTS];
static int g_proxy_running = 0;
static socket_t g_listen_sock = INVALID_SOCKET_VAL;
static HANDLE g_proxy_thread = NULL;
static CRITICAL_SECTION g_proxy_lock;
static int g_proxy_port = DEFAULT_PROXY_PORT;

static unsigned int __stdcall proxy_worker(void *arg)
{
    (void)arg;

    while (g_proxy_running)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(g_listen_sock, &read_fds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int select_result = select(0, &read_fds, NULL, NULL, &tv);

        if (select_result > 0 && FD_ISSET(g_listen_sock, &read_fds))
        {
            struct sockaddr_in client_addr;
            int addr_len = sizeof(client_addr);
            socket_t client_sock = accept(g_listen_sock, (struct sockaddr *)&client_addr, &addr_len);

            if (client_sock != INVALID_SOCKET_VAL)
            {
                LOG_INFO("Proxy client connected from %s:%d",
                         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                close_socket(client_sock);
            }
        }
    }

    return 0;
}

int proxy_server_start(int port)
{
    if (g_proxy_running)
    {
        LOG_WARN("Proxy server already running");
        return 0;
    }

    g_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen_sock == INVALID_SOCKET_VAL)
    {
        LOG_ERROR("Failed to create proxy socket: %d", WSAGetLastError());
        return -1;
    }

    int opt = 1;
    setsockopt(g_listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(g_listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to bind proxy port %d: %d", port, WSAGetLastError());
        close_socket(g_listen_sock);
        return -1;
    }

    if (listen(g_listen_sock, SOMAXCONN) == SOCKET_ERROR)
    {
        LOG_ERROR("Failed to listen on proxy port %d: %d", port, WSAGetLastError());
        close_socket(g_listen_sock);
        return -1;
    }

    InitializeCriticalSection(&g_proxy_lock);
    memset(g_clients, 0, sizeof(g_clients));
    g_proxy_running = 1;
    g_proxy_port = port;

    g_proxy_thread = (HANDLE)_beginthreadex(NULL, 0, proxy_worker, NULL, 0, NULL);

    LOG_INFO("Proxy server started on port %d", port);
    return 0;
}

void proxy_server_stop(void)
{
    if (!g_proxy_running)
        return;

    g_proxy_running = 0;

    if (g_listen_sock != INVALID_SOCKET_VAL)
    {
        close_socket(g_listen_sock);
        g_listen_sock = INVALID_SOCKET_VAL;
    }

    if (g_proxy_thread)
    {
        WaitForSingleObject(g_proxy_thread, 5000);
        CloseHandle(g_proxy_thread);
        g_proxy_thread = NULL;
    }

    DeleteCriticalSection(&g_proxy_lock);
    LOG_INFO("Proxy server stopped");
}

int proxy_server_status(void)
{
    return g_proxy_running;
}

void proxy_server_print_stats(void)
{
    LOG_INFO("========== PROXY SERVER STATS ==========");
    LOG_INFO("Status: %s", g_proxy_running ? "RUNNING" : "STOPPED");
    LOG_INFO("Port: %d", g_proxy_port);
    LOG_INFO("========================================");
}

int proxy_forward_data(socket_t from_sock, socket_t to_sock, uint8_t *buffer, size_t buffer_size)
{
    int received = recv(from_sock, (char *)buffer, (int)buffer_size, 0);
    if (received <= 0)
        return -1;

    int sent = send(to_sock, (char *)buffer, received, 0);
    if (sent != received)
        return -1;

    return received;
}
