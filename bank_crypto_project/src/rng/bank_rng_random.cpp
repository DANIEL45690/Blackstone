#include "bank/bank_rng.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include "bank/bank_platform.h"
#include <string.h>

extern bank_rng_state g_rng_state;
extern int g_library_initialized;

#if defined(_WIN32) || defined(_WIN64)
extern CRITICAL_SECTION g_rng_mutex;
#else
extern pthread_mutex_t g_rng_mutex;
#endif

int bank_random_bytes(uint8_t *buf, size_t len)
{
    if (!buf || len == 0)
        return 0;
    return bank_platform_random_bytes(buf, len);
}

int bank_rng_bytes(uint8_t *buf, size_t len)
{
    if (!g_library_initialized)
        return 0;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_rng_mutex);
#else
    pthread_mutex_lock(&g_rng_mutex);
#endif

    if (g_rng_state.entropy_bits < len * 8)
    {
        bank_platform_random_bytes(g_rng_state.pool, BANK_RNG_POOL_SIZE);
        g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
    }

    size_t remaining = len;
    size_t offset = 0;

    while (remaining > 0)
    {
        size_t to_copy = remaining;
        if (to_copy > BANK_RNG_POOL_SIZE - g_rng_state.pool_index)
        {
            to_copy = BANK_RNG_POOL_SIZE - g_rng_state.pool_index;
        }

        memcpy(buf + offset, g_rng_state.pool + g_rng_state.pool_index, to_copy);
        g_rng_state.pool_index += to_copy;
        remaining -= to_copy;
        offset += to_copy;

        if (g_rng_state.pool_index >= BANK_RNG_POOL_SIZE)
        {
            uint8_t new_pool[BANK_RNG_POOL_SIZE];
            bank_sha256(g_rng_state.pool, BANK_RNG_POOL_SIZE, new_pool);
            memcpy(g_rng_state.pool, new_pool, BANK_RNG_POOL_SIZE);
            bank_secure_zero(new_pool, sizeof(new_pool));
            g_rng_state.pool_index = 0;
            g_rng_state.entropy_bits = BANK_SHA256_DIGEST_SIZE * 8;
        }
    }

    g_rng_state.entropy_bits -= len * 8;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_rng_mutex);
#else
    pthread_mutex_unlock(&g_rng_mutex);
#endif

    return 1;
}
