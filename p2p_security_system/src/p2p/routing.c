#include "routing.h"
#include "../core/logger.h"
#include "../core/network.h"

#define MAX_ROUTES 256

static route_entry_t g_routing_table[MAX_ROUTES];
static int g_route_count = 0;
static CRITICAL_SECTION g_routing_lock;
static int g_initialized = 0;

int routing_init(void)
{
    if (g_initialized)
        return 1;

    InitializeCriticalSection(&g_routing_lock);
    memset(g_routing_table, 0, sizeof(g_routing_table));
    g_route_count = 0;
    g_initialized = 1;

    LOG_INFO("Routing subsystem initialized");
    return 1;
}

void routing_cleanup(void)
{
    if (!g_initialized)
        return;

    DeleteCriticalSection(&g_routing_lock);
    g_initialized = 0;
    LOG_INFO("Routing subsystem cleaned up");
}

int routing_add_route(uint32_t dest_ip, int dest_port, int next_hop, int metric)
{
    EnterCriticalSection(&g_routing_lock);

    for (int i = 0; i < g_route_count; i++)
    {
        if (g_routing_table[i].dest_ip == dest_ip && g_routing_table[i].dest_port == dest_port)
        {
            g_routing_table[i].next_hop_id = next_hop;
            g_routing_table[i].metric = metric;
            g_routing_table[i].last_update = get_timestamp_ms();
            g_routing_table[i].valid = 1;
            LeaveCriticalSection(&g_routing_lock);
            LOG_INFO("Route updated: %d.%d.%d.%d:%d -> node %d (metric %d)",
                     ((uint8_t *)&dest_ip)[0], ((uint8_t *)&dest_ip)[1],
                     ((uint8_t *)&dest_ip)[2], ((uint8_t *)&dest_ip)[3],
                     dest_port, next_hop, metric);
            return 1;
        }
    }

    if (g_route_count < MAX_ROUTES)
    {
        g_routing_table[g_route_count].dest_ip = dest_ip;
        g_routing_table[g_route_count].dest_port = dest_port;
        g_routing_table[g_route_count].next_hop_id = next_hop;
        g_routing_table[g_route_count].metric = metric;
        g_routing_table[g_route_count].last_update = get_timestamp_ms();
        g_routing_table[g_route_count].valid = 1;
        g_route_count++;

        LOG_INFO("Route added: %d.%d.%d.%d:%d -> node %d (metric %d)",
                 ((uint8_t *)&dest_ip)[0], ((uint8_t *)&dest_ip)[1],
                 ((uint8_t *)&dest_ip)[2], ((uint8_t *)&dest_ip)[3],
                 dest_port, next_hop, metric);
    }

    LeaveCriticalSection(&g_routing_lock);
    return 0;
}

int routing_remove_route(uint32_t dest_ip, int dest_port)
{
    EnterCriticalSection(&g_routing_lock);

    for (int i = 0; i < g_route_count; i++)
    {
        if (g_routing_table[i].dest_ip == dest_ip && g_routing_table[i].dest_port == dest_port)
        {
            g_routing_table[i].valid = 0;
            LeaveCriticalSection(&g_routing_lock);
            LOG_INFO("Route removed: %d.%d.%d.%d:%d",
                     ((uint8_t *)&dest_ip)[0], ((uint8_t *)&dest_ip)[1],
                     ((uint8_t *)&dest_ip)[2], ((uint8_t *)&dest_ip)[3], dest_port);
            return 1;
        }
    }

    LeaveCriticalSection(&g_routing_lock);
    return 0;
}

route_entry_t *routing_find_route(uint32_t dest_ip, int dest_port)
{
    EnterCriticalSection(&g_routing_lock);

    for (int i = 0; i < g_route_count; i++)
    {
        if (g_routing_table[i].valid &&
            g_routing_table[i].dest_ip == dest_ip &&
            g_routing_table[i].dest_port == dest_port)
        {
            LeaveCriticalSection(&g_routing_lock);
            return &g_routing_table[i];
        }
    }

    LeaveCriticalSection(&g_routing_lock);
    return NULL;
}

void routing_print_table(void)
{
    EnterCriticalSection(&g_routing_lock);

    LOG_INFO("========== ROUTING TABLE ==========");
    LOG_INFO("Dest IP:Port -> Next Hop (Metric)");

    for (int i = 0; i < g_route_count; i++)
    {
        if (g_routing_table[i].valid)
        {
            LOG_INFO("%d.%d.%d.%d:%d -> node %d (metric %d)",
                     ((uint8_t *)&g_routing_table[i].dest_ip)[0],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[1],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[2],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[3],
                     g_routing_table[i].dest_port,
                     g_routing_table[i].next_hop_id,
                     g_routing_table[i].metric);
        }
    }

    LOG_INFO("===================================");
    LeaveCriticalSection(&g_routing_lock);
}

void routing_update_tables(void)
{
    uint64_t now = get_timestamp_ms();

    EnterCriticalSection(&g_routing_lock);

    for (int i = 0; i < g_route_count; i++)
    {
        if (g_routing_table[i].valid && (now - g_routing_table[i].last_update) > 60000)
        {
            g_routing_table[i].valid = 0;
            LOG_INFO("Route expired: %d.%d.%d.%d:%d",
                     ((uint8_t *)&g_routing_table[i].dest_ip)[0],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[1],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[2],
                     ((uint8_t *)&g_routing_table[i].dest_ip)[3],
                     g_routing_table[i].dest_port);
        }
    }

    LeaveCriticalSection(&g_routing_lock);
}

int routing_get_next_hop(uint32_t dest_ip, int dest_port)
{
    route_entry_t *route = routing_find_route(dest_ip, dest_port);
    if (route)
    {
        return route->next_hop_id;
    }
    return -1;
}
