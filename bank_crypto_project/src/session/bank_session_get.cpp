#include "bank/bank_session.h"
#include "bank/bank_utils.h"
#include "bank/bank_log.h"
#include <string.h>

extern bank_session_manager g_session_mgr;

#if defined(_WIN32) || defined(_WIN64)
extern CRITICAL_SECTION g_session_mutex;
#else
extern pthread_mutex_t g_session_mutex;
#endif

int bank_get_session_key(int session_id, uint8_t *session_key, uint8_t *hmac_key)
{
    if (!g_ctx.initialized)
        return 0;
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return 0;

    int idx = session_id - 1;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    if (!g_session_mgr.keys[idx].active)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        return 0;
    }

    if (time(NULL) > g_session_mgr.keys[idx].expires_at)
    {
        g_session_mgr.keys[idx].active = 0;
        bank_secure_zero(g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);
        g_session_mgr.count--;
        g_session_mgr.total_sessions_destroyed++;
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        BANK_LOG_WARN("Session %d expired", session_id);
        return 0;
    }

    if (session_key)
        memcpy(session_key, g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
    if (hmac_key)
        memcpy(hmac_key, g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);

    g_session_mgr.keys[idx].last_used_at = time(NULL);
    g_session_mgr.keys[idx].use_count++;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif

    return 1;
}

int bank_session_exists(int session_id)
{
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return 0;

    int idx = session_id - 1;
    int exists = 0;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    exists = g_session_mgr.keys[idx].active;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif

    return exists;
}

int bank_session_get_info(int session_id, time_t *created_at, time_t *expires_at, uint64_t *use_count)
{
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return 0;

    int idx = session_id - 1;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    if (!g_session_mgr.keys[idx].active)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        return 0;
    }

    if (created_at)
        *created_at = g_session_mgr.keys[idx].created_at;
    if (expires_at)
        *expires_at = g_session_mgr.keys[idx].expires_at;
    if (use_count)
        *use_count = g_session_mgr.keys[idx].use_count;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif

    return 1;
}
