#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>

int compute_hash_secure(CryptoContext *ctx, const byte *data, size_t data_len, byte *hash_out)
{
    if (!ctx || !ctx->initialized || !data || !hash_out)
        return CRYPTO_ERROR;

    lock_critical_section(ctx);

    memcpy(ctx->binary_buffer, data, data_len < 4096 ? data_len : 4096);
    hash_core(ctx, ctx->binary_buffer, data_len < 4096 ? data_len : 4096);
    memcpy(hash_out, ctx->hash_buffer, HASH_SIZE);

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

void hash_core(CryptoContext *ctx, const byte *data, size_t data_len)
{
    size_t offset, round;
    u64 a, b, c, d, e, f, g, h;

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (offset = 0; offset + 64 <= data_len; offset += 64)
    {
        const u64 *block = (const u64 *)(data + offset);

        for (round = 0; round < 8; round++)
        {
            a ^= block[round];
            a = (a << 13) | (a >> 51);
            b ^= a;
            b = (b << 11) | (b >> 53);
            c ^= b;
            c = (c << 7) | (c >> 57);
            d ^= c;
            d = (d << 5) | (d >> 59);
            e ^= d;
            e = (e << 3) | (e >> 61);
            f ^= e;
            f = (f << 2) | (f >> 62);
            g ^= f;
            g = (g << 1) | (g >> 63);
            h ^= g;
        }
    }

    ctx->state[0] = a;
    ctx->state[1] = b;
    ctx->state[2] = c;
    ctx->state[3] = d;
    ctx->state[4] = e;
    ctx->state[5] = f;
    ctx->state[6] = g;
    ctx->state[7] = h;

    memcpy(ctx->hash_buffer, ctx->state, HASH_SIZE);
}
