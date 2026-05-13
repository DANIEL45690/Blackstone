#include "auth.h"
#include "../core/logger.h"
#include "../core/crypto.h"

#define MAX_USERS 64

static auth_user_t g_users[MAX_USERS];
static int g_user_count = 0;
static CRITICAL_SECTION g_auth_lock;
static int g_initialized = 0;

int auth_init(void)
{
    if (g_initialized)
        return 1;

    InitializeCriticalSection(&g_auth_lock);
    memset(g_users, 0, sizeof(g_users));
    g_user_count = 0;
    g_initialized = 1;

    auth_add_user("admin", "admin123", 1);
    auth_add_user("user", "user123", 0);

    LOG_INFO("Authentication system initialized");
    return 1;
}

void auth_cleanup(void)
{
    if (!g_initialized)
        return;

    DeleteCriticalSection(&g_auth_lock);
    g_initialized = 0;
    LOG_INFO("Authentication system cleaned up");
}

int auth_add_user(const char *username, const char *password, int role)
{
    EnterCriticalSection(&g_auth_lock);

    for (int i = 0; i < g_user_count; i++)
    {
        if (strcmp(g_users[i].username, username) == 0)
        {
            sha256((const uint8_t *)password, strlen(password), g_users[i].password_hash);
            g_users[i].role = role;
            g_users[i].enabled = 1;
            LeaveCriticalSection(&g_auth_lock);
            LOG_INFO("User updated: %s (role %d)", username, role);
            return 1;
        }
    }

    if (g_user_count < MAX_USERS)
    {
        strcpy(g_users[g_user_count].username, username);
        sha256((const uint8_t *)password, strlen(password), g_users[g_user_count].password_hash);
        g_users[g_user_count].role = role;
        g_users[g_user_count].enabled = 1;
        g_users[g_user_count].last_login = 0;
        g_user_count++;

        LOG_INFO("User added: %s (role %d)", username, role);
    }

    LeaveCriticalSection(&g_auth_lock);
    return 0;
}

int auth_remove_user(const char *username)
{
    EnterCriticalSection(&g_auth_lock);

    for (int i = 0; i < g_user_count; i++)
    {
        if (strcmp(g_users[i].username, username) == 0)
        {
            for (int j = i; j < g_user_count - 1; j++)
            {
                g_users[j] = g_users[j + 1];
            }
            g_user_count--;
            LeaveCriticalSection(&g_auth_lock);
            LOG_INFO("User removed: %s", username);
            return 1;
        }
    }

    LeaveCriticalSection(&g_auth_lock);
    return 0;
}

int auth_verify(const char *username, const char *password)
{
    EnterCriticalSection(&g_auth_lock);

    uint8_t hash[SHA256_DIGEST_SIZE];
    sha256((const uint8_t *)password, strlen(password), hash);

    for (int i = 0; i < g_user_count; i++)
    {
        if (strcmp(g_users[i].username, username) == 0 && g_users[i].enabled)
        {
            if (secure_compare(g_users[i].password_hash, hash, SHA256_DIGEST_SIZE))
            {
                g_users[i].last_login = get_timestamp_ms();
                LeaveCriticalSection(&g_auth_lock);
                LOG_INFO("User authenticated: %s", username);
                return 1;
            }
        }
    }

    LeaveCriticalSection(&g_auth_lock);
    LOG_WARN("Authentication failed for user: %s", username);
    return 0;
}

int auth_login(int node_id, const char *username, const char *password)
{
    (void)node_id;
    return auth_verify(username, password);
}

void auth_logout(int node_id)
{
    (void)node_id;
    LOG_INFO("Node %d logged out", node_id);
}

void auth_print_users(void)
{
    EnterCriticalSection(&g_auth_lock);

    LOG_INFO("========== AUTH USERS ==========");
    LOG_INFO("Username | Role | Enabled | Last Login");

    for (int i = 0; i < g_user_count; i++)
    {
        LOG_INFO("%s | %d | %d | %llu",
                 g_users[i].username,
                 g_users[i].role,
                 g_users[i].enabled,
                 (unsigned long long)g_users[i].last_login);
    }

    LOG_INFO("================================");
    LeaveCriticalSection(&g_auth_lock);
}
