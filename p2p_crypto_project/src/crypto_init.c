#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>
#include <immintrin.h>

int crypto_init(CryptoContext *ctx)
{
    if (!ctx)
        return CRYPTO_ERROR;

    if (ctx->initialized)
        return CRYPTO_SUCCESS;

    memset(ctx, 0, sizeof(CryptoContext));

    init_sboxes(ctx);
    init_gf256_tables(ctx);
    init_permutation(ctx);
    init_round_constants(ctx);
    init_entropy_pool(ctx);

    ctx->state[0] = rdtsc_wrapper();
    ctx->state[0] ^= rdrand_wrapper();
    ctx->state[1] ^= rdseed_wrapper();

    collect_system_entropy(ctx);

    ctx->initialized = 1;
    return CRYPTO_SUCCESS;
}

int crypto_shutdown(CryptoContext *ctx)
{
    if (!ctx)
        return CRYPTO_ERROR;

    wipe_all_secure_memory(ctx);
    ctx->initialized = 0;

    return CRYPTO_SUCCESS;
}

u64 rdrand_wrapper(void)
{
    u64 val;
    unsigned char retry = 0;
    while (!_rdrand64_step(&val) && retry < 10)
        retry++;
    return val;
}

u64 rdseed_wrapper(void)
{
    u64 val;
    unsigned char retry = 0;
    while (!_rdseed64_step(&val) && retry < 10)
        retry++;
    return val;
}

u64 rdtsc_wrapper(void)
{
    return __rdtsc();
}
