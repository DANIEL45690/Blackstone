#include "crypto_core.h"

void init_gf256_tables(CryptoContext *ctx)
{
    int i;
    byte x = 1;

    for (i = 0; i < 255; i++)
    {
        ctx->gf256_exp[i] = x;
        x <<= 1;
        if (x & 0x80)
            x ^= 0x1B;
    }
    ctx->gf256_exp[255] = x;

    for (i = 0; i < 256; i++)
    {
        ctx->gf256_log[ctx->gf256_exp[i]] = i;
    }
}
