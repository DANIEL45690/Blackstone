#include "bank/bank_crypto.h"
#include "bank/internal/bank_memory.h"
#include "bank/bank_platform.h"

static bank_crypto_context g_ctx = {0};
static bank_session_manager g_session_mgr = {0};
static bank_rng_state g_rng_state = {0};
static int g_library_initialized = 0;
static uint64_t g_library_start_time = 0;

#if defined(_WIN32) || defined(_WIN64)
static CRITICAL_SECTION g_mutex;
static CRITICAL_SECTION g_rng_mutex;
static CRITICAL_SECTION g_session_mutex;
#else
static pthread_mutex_t g_mutex;
static pthread_mutex_t g_rng_mutex;
static pthread_mutex_t g_session_mutex;
#endif

int bank_init_master_random(void)
{
    if (!bank_random_bytes(g_ctx.master_key, BANK_AES_KEY_BYTES))
        return 0;
    if (!bank_random_bytes(g_ctx.salt, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }
    if (!bank_random_bytes(g_ctx.anti_tamper_seed, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }
    if (!bank_random_bytes(g_ctx.key_rotation_seed, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }

#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&g_mutex);
    InitializeCriticalSection(&g_rng_mutex);
    InitializeCriticalSection(&g_session_mutex);
#else
    pthread_mutex_init(&g_mutex, NULL);
    pthread_mutex_init(&g_rng_mutex, NULL);
    pthread_mutex_init(&g_session_mutex, NULL);
#endif

    memcpy(g_ctx.key_chain[0], g_ctx.master_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = 0;
    g_ctx.key_created_at = time(NULL);
    g_ctx.total_encryptions = 0;
    g_ctx.total_decryptions = 0;
    g_ctx.initialized = 1;
    g_ctx.ref_count = 1;
    g_library_initialized = 1;
    g_library_start_time = time(NULL);
    g_session_mgr.count = 0;
    g_session_mgr.total_sessions_created = 0;
    g_session_mgr.total_sessions_destroyed = 0;

    bank_rng_init();

    BANK_LOG_INFO("Cryptographic module initialized with random master key");
    return 1;
}

int bank_init_master(const uint8_t *key, size_t key_len, const uint8_t *salt, size_t salt_len)
{
    if (key_len != BANK_AES_KEY_BYTES || salt_len != 32)
        return 0;

    memcpy(g_ctx.master_key, key, BANK_AES_KEY_BYTES);
    memcpy(g_ctx.salt, salt, 32);
    bank_random_bytes(g_ctx.anti_tamper_seed, 32);
    bank_random_bytes(g_ctx.key_rotation_seed, 32);

#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&g_mutex);
    InitializeCriticalSection(&g_rng_mutex);
    InitializeCriticalSection(&g_session_mutex);
#else
    pthread_mutex_init(&g_mutex, NULL);
    pthread_mutex_init(&g_rng_mutex, NULL);
    pthread_mutex_init(&g_session_mutex, NULL);
#endif

    memcpy(g_ctx.key_chain[0], g_ctx.master_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = 0;
    g_ctx.key_created_at = time(NULL);
    g_ctx.total_encryptions = 0;
    g_ctx.total_decryptions = 0;
    g_ctx.initialized = 1;
    g_ctx.ref_count = 1;
    g_library_initialized = 1;
    g_library_start_time = time(NULL);
    g_session_mgr.count = 0;

    bank_rng_init();

    BANK_LOG_INFO("Cryptographic module initialized with provided master key");
    return 1;
}

void bank_rotate_master_key(void)
{
    if (!g_ctx.initialized)
        return;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_mutex);
#else
    pthread_mutex_lock(&g_mutex);
#endif

    int new_index = (g_ctx.current_key_index + 1) % BANK_MAX_KEY_ROTATIONS;
    uint8_t new_key[BANK_AES_KEY_BYTES];
    uint8_t hash_input[64];

    memcpy(hash_input, g_ctx.key_chain[g_ctx.current_key_index], BANK_AES_KEY_BYTES);
    memcpy(hash_input + BANK_AES_KEY_BYTES, g_ctx.key_rotation_seed, 32);
    bank_sha256(hash_input, BANK_AES_KEY_BYTES + 32, new_key);
    memcpy(g_ctx.key_chain[new_index], new_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = new_index;
    g_ctx.key_created_at = time(NULL);

    bank_secure_zero(new_key, sizeof(new_key));
    bank_secure_zero(hash_input, sizeof(hash_input));

    BANK_LOG_INFO("Master key rotated to index %d", new_index);

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_mutex);
#else
    pthread_mutex_unlock(&g_mutex);
#endif
}

void bank_wipe_master(void)
{
    if (!g_ctx.initialized)
        return;

    bank_secure_zero(&g_ctx, sizeof(g_ctx));
    bank_secure_zero(&g_session_mgr, sizeof(g_session_mgr));

#if defined(_WIN32) || defined(_WIN64)
    DeleteCriticalSection(&g_mutex);
    DeleteCriticalSection(&g_rng_mutex);
    DeleteCriticalSection(&g_session_mutex);
#else
    pthread_mutex_destroy(&g_mutex);
    pthread_mutex_destroy(&g_rng_mutex);
    pthread_mutex_destroy(&g_session_mutex);
#endif

    g_ctx.initialized = 0;
    g_library_initialized = 0;

    BANK_LOG_INFO("Cryptographic module wiped");
}

int bank_is_initialized(void)
{
    return g_library_initialized;
}

void bank_get_stats(uint64_t *total_encryptions, uint64_t *total_decryptions,
                    uint64_t *total_sessions_created, uint64_t *total_sessions_destroyed,
                    int *active_sessions)
{
    if (total_encryptions)
        *total_encryptions = g_ctx.total_encryptions;
    if (total_decryptions)
        *total_decryptions = g_ctx.total_decryptions;
    if (total_sessions_created)
        *total_sessions_created = g_session_mgr.total_sessions_created;
    if (total_sessions_destroyed)
        *total_sessions_destroyed = g_session_mgr.total_sessions_destroyed;
    if (active_sessions)
        *active_sessions = g_session_mgr.count;
}

time_t bank_get_library_uptime(void)
{
    if (!g_library_initialized)
        return 0;
    return time(NULL) - g_library_start_time;
}
