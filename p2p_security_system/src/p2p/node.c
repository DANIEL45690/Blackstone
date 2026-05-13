#include "node.h"
#include "../core/logger.h"
#include "../core/network.h"
#include <stdlib.h>

static int g_initialized = 0;

int node_init(void)
{
    if (g_initialized)
        return 1;
    g_initialized = 1;
    LOG_INFO("P2P node subsystem initialized");
    return 1;
}

void node_cleanup(void)
{
    if (!g_initialized)
        return;
    g_initialized = 0;
    LOG_INFO("P2P node subsystem cleaned up");
}

int node_create(const char *ip, int port)
{
    (void)ip;
    int node_id = network_create_node(port);
    if (node_id >= 0)
    {
        LOG_INFO("P2P node created: ID=%d, port=%d", node_id, port);
    }
    return node_id;
}

int node_connect(int node_id, const char *target_ip, int target_port)
{
    int result = network_connect_node(node_id, target_ip, target_port);
    if (result == 0)
    {
        result = network_handshake(node_id);
        LOG_INFO("P2P node %d connected to %s:%d, handshake=%s",
                 node_id, target_ip, target_port, result ? "OK" : "FAIL");
    }
    return result;
}

int node_disconnect(int node_id)
{
    node_t *node = network_get_node(node_id);
    if (!node)
        return -1;

    EnterCriticalSection(&node->lock);
    node->connected = 0;
    if (node->socket != INVALID_SOCKET_VAL)
    {
        close_socket(node->socket);
        node->socket = INVALID_SOCKET_VAL;
    }
    LeaveCriticalSection(&node->lock);

    LOG_INFO("P2P node %d disconnected", node_id);
    return 0;
}

int node_send(int node_id, const uint8_t *data, size_t len)
{
    return network_send_packet(node_id, PACKET_DATA, data, len);
}

int node_broadcast(const uint8_t *data, size_t len)
{
    int count = network_get_node_count();
    int success = 0;

    for (int i = 0; i < count; i++)
    {
        if (network_send_packet(i, PACKET_DATA, data, len) > 0)
        {
            success++;
        }
    }

    LOG_INFO("Broadcast to %d nodes, %d successful", count, success);
    return success;
}

void node_update_status(void)
{
    network_check_heartbeats();
}

void node_print_all(void)
{
    network_print_stats();
}
