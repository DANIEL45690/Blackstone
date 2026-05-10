#include "bank/bank_rng.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include "bank/bank_platform.h"
#include <string.h>

extern bank_rng_state g_rng_state;
extern int g_library_initialized;

void bank_rng_init(void)
{
    bank_platform_random_bytes(g_rng_state.pool, BANK_RNG_POOL_SIZE);
    g_rng_state.pool_index = 0;
    g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
}
