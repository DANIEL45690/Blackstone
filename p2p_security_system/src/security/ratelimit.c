#include "ratelimit.h"
#include "../core/logger.h"

#define MAX_RATELIMIT_ENTRIES 1024

static rate_limit_entry_t g_rate_entries[MAX_RATELIMIT_ENTRIES];
static int g_rate_count = 0;
static CRITICAL_SECTION g_rate_lock;
static int g_initialized = 0;

int ratelimit_init(void)
{
    if (g_initialized)
        return 1;

    InitializeCriticalSection(&g_rate_lock);
    memset(g_rate_entries, 0, sizeof(g_rate_entries));
    g_rate_count = 0;
    g_initialized = 1;

    LOG_INFO("Rate limiting initialized");
    return 1;
}

void ratelimit_cleanup(void)
{
    if (!g_initialized)
        return;

    DeleteCriticalSection(&g_rate_lock);
    g_initialized = 0;
    LOG_INFO("Rate limiting cleaned up");
}

int ratelimit_check(const char *ip, int max_requests, uint64_t window_ms)
{
    uint64_t now = get_timestamp_ms();

    EnterCriticalSection(&g_rate_lock);

    for (int i = 0; i < g_rate_count; i++)
    {
        if (strcmp(g_rate_entries[i].ip, ip) == 0)
        {
            if ((now - g_rate_entries[i].window_start) > window_ms)
            {
                g_rate_entries[i].count = 0;
                g_rate_entries[i].window_start = now;
            }

            g_rate_entries[i].timestamps[g_rate_entries[i].count % 100] = now;
            g_rate_entries[i].count++;

            int result = g_rate_entries[i].count <= max_requests;

            if (!result)
            {
                LOG_WARN("Rate limit exceeded for %s: %d requests in %llu ms",
                         ip, g_rate_entries[i].count, (unsigned long long)window_ms);
            }

            LeaveCriticalSection(&g_rate_lock);
            return result;
        }
    }

    if (g_rate_count < MAX_RATELIMIT_ENTRIES)
    {
        strcpy(g_rate_entries[g_rate_count].ip, ip);
        g_rate_entries[g_rate_count].timestamps[0] = now;
        g_rate_entries[g_rate_count].count = 1;
        g_rate_entries[g_rate_count].window_start = now;
        g_rate_count++;
    }

    LeaveCriticalSection(&g_rate_lock);
    return 1;
}

void ratelimit_reset(const char *ip)
{
    EnterCriticalSection(&g_rate_lock);

    for (int i = 0; i < g_rate_count; i++)
    {
        if (strcmp(g_rate_entries[i].ip, ip) == 0)
        {
            g_rate_entries[i].count = 0;
            g_rate_entries[i].window_start = get_timestamp_ms();
            break;
        }
    }

    LeaveCriticalSection(&g_rate_lock);
    LOG_INFO("Rate limit reset for %s", ip);
}

void ratelimit_cleanup_old(void)
{
    uint64_t now = get_timestamp_ms();

    EnterCriticalSection(&g_rate_lock);

    int new_count = 0;
    for (int i = 0; i < g_rate_count; i++)
    {
        if ((now - g_rate_entries[i].window_start) < 60000)
        {
            if (new_count != i)
            {
                g_rate_entries[new_count] = g_rate_entries[i];
            }
            new_count++;
        }
    }
    g_rate_count = new_count;

    LeaveCriticalSection(&g_rate_lock);
}

void ratelimit_print_stats(void)
{
    EnterCriticalSection(&g_rate_lock);

    LOG_INFO("========== RATE LIMIT STATS ==========");
    LOG_INFO("IP | Count | Window Start");

    for (int i = 0; i < g_rate_count; i++)
    {
        LOG_INFO("%s | %d | %llu",
                 g_rate_entries[i].ip,
                 g_rate_entries[i].count,
                 (unsigned long long)g_rate_entries[i].window_start);
    }

    LOG_INFO("=====================================");
    LeaveCriticalSection(&g_rate_lock);
}
