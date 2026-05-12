#include "crypto_core.h"
#include <string.h>

void init_sboxes(CryptoContext *ctx)
{
    int i;
    byte temp;
    u64 rnd;

    for (i = 0; i < 256; i++)
    {
        ctx->sbox_byte[i] = i;
    }

    for (i = 255; i > 0; i--)
    {
        rnd = rdrand_wrapper();
        int j = (rnd & 0xFF) % (i + 1);
        temp = ctx->sbox_byte[i];
        ctx->sbox_byte[i] = ctx->sbox_byte[j];
        ctx->sbox_byte[j] = temp;
    }

    for (i = 0; i < 256; i++)
    {
        ctx->inv_sbox[ctx->sbox_byte[i]] = i;
    }

    for (i = 0; i < 256; i++)
    {
        byte val = i;
        val ^= 0x5A;
        val = (val << 3) | (val >> 5);
        val ^= 0x3C;
        val = (val >> 2) | (val << 6);
        ctx->ternary_sbox[i] = val;
    }

    for (i = 0; i < 256; i++)
    {
        ctx->ternary_inv_sbox[ctx->ternary_sbox[i]] = i;
    }
}
