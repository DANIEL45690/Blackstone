#include "bank/bank_session.h"
#include "bank/bank_pbkdf2.h"
#include "bank/bank_rng.h"
#include "bank/bank_utils.h"
#include "bank/bank_log.h"
#include <string.h>

extern bank_crypto_context g_ctx;
extern bank_session_manager g_session_mgr;

#if defined(_WIN32) || defined(_WIN64)
extern CRITICAL_SECTION g_session_mutex;
#else
extern pthread_mutex_t g_session_mutex;
#endif

static int bank_derive_session_key(const uint8_t *key_id, size_t key_id_len, uint8_t *session_key, uint8_t *hmac_key)
{
    uint8_t data[64];
    uint8_t derived[BANK_AES_KEY_BYTES * 2];

    if (key_id_len > 32)
        key_id_len = 32;

    memcpy(data, g_ctx.salt, 32);
    memcpy(data + 32, key_id, key_id_len);

    bank_pbkdf2_hmac_sha256(g_ctx.key_chain[g_ctx.current_key_index], BANK_AES_KEY_BYTES,
                            data, 32 + key_id_len, BANK_PBKDF2_ROUNDS, derived, BANK_AES_KEY_BYTES * 2);

    memcpy(session_key, derived, BANK_AES_KEY_BYTES);
    memcpy(hmac_key, derived + BANK_AES_KEY_BYTES, BANK_AES_KEY_BYTES);
    bank_secure_zero(derived, sizeof(derived));

    return 1;
}

int bank_create_session(const uint8_t *key_id, size_t key_id_len, int ttl_seconds)
{
    if (!g_ctx.initialized)
        return 0;
    if (key_id_len > BANK_MAX_KEY_ID)
        return 0;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mutex);
#else
    pthread_mutex_lock(&g_session_mutex);
#endif

    if (g_session_mgr.count >= BANK_MAX_SESSION_KEYS)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        BANK_LOG_WARN("Session creation failed: max sessions reached");
        return 0;
    }

    int slot = -1;
    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (!g_session_mgr.keys[i].active)
        {
            slot = i;
            break;
        }
    }

    if (slot == -1)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        return 0;
    }

    if (!bank_derive_session_key(key_id, key_id_len, g_session_mgr.keys[slot].session_key,
                                 g_session_mgr.keys[slot].hmac_key))
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mutex);
#else
        pthread_mutex_unlock(&g_session_mutex);
#endif
        return 0;
    }

    memcpy(g_session_mgr.keys[slot].key_id, key_id, key_id_len);
    g_session_mgr.keys[slot].key_id_len = key_id_len;
    g_session_mgr.keys[slot].created_at = time(NULL);
    g_session_mgr.keys[slot].expires_at = time(NULL) + ttl_seconds;
    g_session_mgr.keys[slot].last_used_at = time(NULL);
    g_session_mgr.keys[slot].use_count = 0;
    g_session_mgr.keys[slot].active = 1;
    bank_random_bytes(g_session_mgr.keys[slot].salt, 16);
    g_session_mgr.count++;
    g_session_mgr.total_sessions_created++;

    BANK_LOG_INFO("Session created with ID %d, TTL %d seconds", slot + 1, ttl_seconds);

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mutex);
#else
    pthread_mutex_unlock(&g_session_mutex);
#endif

    return slot + 1;
}
