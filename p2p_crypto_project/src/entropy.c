#include "crypto_core.h"
#include <cpuid.h>

void init_entropy_pool(CryptoContext *ctx)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        ctx->entropy_pool[i] = rdrand_wrapper();
        ctx->entropy_pool[i] ^= rdseed_wrapper();
        ctx->entropy_pool[i] ^= rdtsc_wrapper();
    }
}

void collect_system_entropy(CryptoContext *ctx)
{
    u32 eax, ebx, ecx, edx;

    __cpuid(0x80000001, eax, ebx, ecx, edx);
    ctx->entropy_pool[0] ^= ((u64)eax | ((u64)ebx << 32));
    ctx->entropy_pool[1] ^= ((u64)ecx | ((u64)edx << 32));

    u64 tsc = rdtsc_wrapper();
    ctx->entropy_pool[4] ^= tsc;

    __cpuid(0x0F, eax, ebx, ecx, edx);
    ctx->entropy_pool[6] ^= ((u64)eax | ((u64)ebx << 32));
}

int entropy_add(CryptoContext *ctx, const byte *data, size_t data_len)
{
    size_t i;
    if (!ctx || !data)
        return CRYPTO_ERROR;

    for (i = 0; i < data_len && i < 32; i++)
    {
        ctx->entropy_pool[i] ^= data[i];
    }

    return CRYPTO_SUCCESS;
}
