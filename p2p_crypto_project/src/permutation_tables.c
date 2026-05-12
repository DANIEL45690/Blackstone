#include "crypto_core.h"

void init_permutation(CryptoContext *ctx)
{
    int i, j;
    u32 temp;
    u64 rnd;

    for (i = 0; i < 256; i++)
    {
        ctx->permutation_table[i] = i;
    }

    for (i = 0; i < 1000; i++)
    {
        rnd = rdrand_wrapper();
        int idx1 = rnd % 256;
        rnd = rdrand_wrapper();
        int idx2 = rnd % 256;

        temp = ctx->permutation_table[idx1];
        ctx->permutation_table[idx1] = ctx->permutation_table[idx2];
        ctx->permutation_table[idx2] = temp;
    }

    for (i = 0; i < 256; i++)
    {
        ctx->inverse_perm[ctx->permutation_table[i]] = i;
    }
}

void init_round_constants(CryptoContext *ctx)
{
    int i;
    for (i = 0; i < 64; i++)
    {
        ctx->round_constants[i] = (u64)i * i * 0x9E3779B97F4A7C15ULL;
    }
}
