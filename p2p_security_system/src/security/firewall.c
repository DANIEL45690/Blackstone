#include "firewall.h"
#include "../core/logger.h"

#define MAX_FW_RULES 256

static firewall_rule_t g_fw_rules[MAX_FW_RULES];
static int g_fw_rule_count = 0;
static CRITICAL_SECTION g_fw_lock;
static int g_initialized = 0;

int firewall_init(void)
{
    if (g_initialized)
        return 1;

    InitializeCriticalSection(&g_fw_lock);
    memset(g_fw_rules, 0, sizeof(g_fw_rules));
    g_fw_rule_count = 0;
    g_initialized = 1;

    LOG_INFO("Firewall initialized");
    return 1;
}

void firewall_cleanup(void)
{
    if (!g_initialized)
        return;

    DeleteCriticalSection(&g_fw_lock);
    g_initialized = 0;
    LOG_INFO("Firewall cleaned up");
}

int firewall_add_rule(const char *ip, int port, firewall_action_t action, int priority)
{
    EnterCriticalSection(&g_fw_lock);

    for (int i = 0; i < g_fw_rule_count; i++)
    {
        if (strcmp(g_fw_rules[i].ip, ip) == 0 && g_fw_rules[i].port == port)
        {
            g_fw_rules[i].action = action;
            g_fw_rules[i].priority = priority;
            LeaveCriticalSection(&g_fw_lock);
            LOG_INFO("Firewall rule updated: %s:%d -> action=%d", ip, port, action);
            return 1;
        }
    }

    if (g_fw_rule_count < MAX_FW_RULES)
    {
        strcpy(g_fw_rules[g_fw_rule_count].ip, ip);
        g_fw_rules[g_fw_rule_count].port = port;
        g_fw_rules[g_fw_rule_count].action = action;
        g_fw_rules[g_fw_rule_count].priority = priority;
        g_fw_rule_count++;

        LOG_INFO("Firewall rule added: %s:%d -> action=%d (priority %d)", ip, port, action, priority);
    }

    LeaveCriticalSection(&g_fw_lock);
    return 0;
}

int firewall_remove_rule(const char *ip, int port)
{
    EnterCriticalSection(&g_fw_lock);

    for (int i = 0; i < g_fw_rule_count; i++)
    {
        if (strcmp(g_fw_rules[i].ip, ip) == 0 && g_fw_rules[i].port == port)
        {
            for (int j = i; j < g_fw_rule_count - 1; j++)
            {
                g_fw_rules[j] = g_fw_rules[j + 1];
            }
            g_fw_rule_count--;
            LeaveCriticalSection(&g_fw_lock);
            LOG_INFO("Firewall rule removed: %s:%d", ip, port);
            return 1;
        }
    }

    LeaveCriticalSection(&g_fw_lock);
    return 0;
}

firewall_action_t firewall_check(const char *ip, int port)
{
    EnterCriticalSection(&g_fw_lock);

    int highest_priority = -999;
    firewall_action_t result = FW_ACTION_ALLOW;

    for (int i = 0; i < g_fw_rule_count; i++)
    {
        if (strcmp(g_fw_rules[i].ip, ip) == 0 && g_fw_rules[i].port == port)
        {
            if (g_fw_rules[i].priority > highest_priority)
            {
                highest_priority = g_fw_rules[i].priority;
                result = g_fw_rules[i].action;
            }
        }
    }

    if (result == FW_ACTION_DENY)
    {
        LOG_WARN("Firewall blocked connection from %s:%d", ip, port);
    }

    LeaveCriticalSection(&g_fw_lock);
    return result;
}

void firewall_print_rules(void)
{
    EnterCriticalSection(&g_fw_lock);

    LOG_INFO("========== FIREWALL RULES ==========");
    LOG_INFO("IP:Port -> Action (Priority)");

    for (int i = 0; i < g_fw_rule_count; i++)
    {
        const char *action_str = "";
        switch (g_fw_rules[i].action)
        {
        case FW_ACTION_ALLOW:
            action_str = "ALLOW";
            break;
        case FW_ACTION_DENY:
            action_str = "DENY";
            break;
        case FW_ACTION_LOG:
            action_str = "LOG";
            break;
        }
        LOG_INFO("%s:%d -> %s (priority %d)",
                 g_fw_rules[i].ip, g_fw_rules[i].port,
                 action_str, g_fw_rules[i].priority);
    }

    LOG_INFO("====================================");
    LeaveCriticalSection(&g_fw_lock);
}

void firewall_clear_rules(void)
{
    EnterCriticalSection(&g_fw_lock);
    g_fw_rule_count = 0;
    LeaveCriticalSection(&g_fw_lock);
    LOG_INFO("Firewall rules cleared");
}
