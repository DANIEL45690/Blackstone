#include "bank/bank_rng.h"
#include "bank/bank_sha.h"
#include "bank/bank_platform.h"
#include <string.h>

extern bank_rng_state g_rng_state;
extern int g_library_initialized;

void bank_rng_reseed(void)
{
    if (!g_library_initialized)
        return;

    uint8_t new_entropy[BANK_RNG_POOL_SIZE];
    bank_platform_random_bytes(new_entropy, BANK_RNG_POOL_SIZE);

    for (size_t i = 0; i < BANK_RNG_POOL_SIZE; i++)
        g_rng_state.pool[i] ^= new_entropy[i];

    bank_secure_zero(new_entropy, sizeof(new_entropy));
    g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
}

void bank_rng_add_entropy(const uint8_t *data, size_t len)
{
    if (!g_library_initialized || !data || len == 0)
        return;

    size_t to_add = len < BANK_RNG_POOL_SIZE ? len : BANK_RNG_POOL_SIZE;
    for (size_t i = 0; i < to_add; i++)
        g_rng_state.pool[i] ^= data[i];

    g_rng_state.entropy_bits += to_add * 8;
    if (g_rng_state.entropy_bits > BANK_RNG_POOL_SIZE * 8)
        g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
}

uint64_t bank_rng_entropy_available(void)
{
    return g_rng_state.entropy_bits;
}
