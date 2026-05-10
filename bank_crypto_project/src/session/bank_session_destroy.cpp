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

void bank_destroy_session(int session_id)
{
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return;

    int idx = session_id - 1;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    if (g_session_mgr.keys[idx].active)
    {
        bank_secure_zero(g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].key_id, BANK_MAX_KEY_ID);
        g_session_mgr.keys[idx].active = 0;
        g_session_mgr.count--;
        g_session_mgr.total_sessions_destroyed++;
        BANK_LOG_INFO("Session %d destroyed", session_id);
    }

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif
}

void bank_cleanup_expired_sessions(void)
{
    time_t now = time(NULL);

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (g_session_mgr.keys[i].active && now > g_session_mgr.keys[i].expires_at)
        {
            bank_secure_zero(g_session_mgr.keys[i].session_key, BANK_AES_KEY_BYTES);
            bank_secure_zero(g_session_mgr.keys[i].hmac_key, BANK_AES_KEY_BYTES);
            g_session_mgr.keys[i].active = 0;
            g_session_mgr.count--;
            g_session_mgr.total_sessions_destroyed++;
            BANK_LOG_INFO("Expired session %d cleaned up", i + 1);
        }
    }

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif
}
