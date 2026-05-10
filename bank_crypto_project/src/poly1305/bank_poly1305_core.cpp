#include "bank/bank_poly1305.h"
#include "bank/bank_utils.h"
#include <string.h>

void bank_poly1305_init(bank_poly1305_ctx *ctx, const uint8_t *key)
{
    ctx->r[0] = ((uint32_t)key[0] | ((uint32_t)key[1] << 8) | ((uint32_t)key[2] << 16) | ((uint32_t)key[3] << 24)) & 0x0ffffffc;
    ctx->r[1] = ((uint32_t)key[4] | ((uint32_t)key[5] << 8) | ((uint32_t)key[6] << 16) | ((uint32_t)key[7] << 24)) & 0x0ffffffc;
    ctx->r[2] = ((uint32_t)key[8] | ((uint32_t)key[9] << 8) | ((uint32_t)key[10] << 16) | ((uint32_t)key[11] << 24)) & 0x0ffffffc;
    ctx->r[3] = ((uint32_t)key[12] | ((uint32_t)key[13] << 8) | ((uint32_t)key[14] << 16) | ((uint32_t)key[15] << 24)) & 0x0ffffffc;

    ctx->s[0] = (uint32_t)key[16] | ((uint32_t)key[17] << 8) | ((uint32_t)key[18] << 16) | ((uint32_t)key[19] << 24);
    ctx->s[1] = (uint32_t)key[20] | ((uint32_t)key[21] << 8) | ((uint32_t)key[22] << 16) | ((uint32_t)key[23] << 24);
    ctx->s[2] = (uint32_t)key[24] | ((uint32_t)key[25] << 8) | ((uint32_t)key[26] << 16) | ((uint32_t)key[27] << 24);
    ctx->s[3] = (uint32_t)key[28] | ((uint32_t)key[29] << 8) | ((uint32_t)key[30] << 16) | ((uint32_t)key[31] << 24);

    memset(ctx->buf, 0, sizeof(ctx->buf));
    ctx->buf_len = 0;
    ctx->total_len = 0;
}

static void bank_poly1305_add_block(bank_poly1305_ctx *ctx, const uint8_t *block, size_t len, int final_block)
{
    uint32_t t[4];
    uint32_t carry;
    uint64_t s0, s1, s2, s3;

    t[0] = 0;
    t[1] = 0;
    t[2] = 0;
    t[3] = 0;

    for (size_t i = 0; i < len; i++)
    {
        t[i >> 2] |= (uint32_t)block[i] << ((i & 3) * 8);
    }

    if (final_block)
    {
        t[0] |= 1 << (len * 8);
    }

    uint32_t h[4];
    memcpy(h, ctx->buf, sizeof(h));

    h[0] += t[0] & 0x3ffffff;
    h[1] += t[1] & 0x3ffffff;
    h[2] += t[2] & 0x3ffffff;
    h[3] += t[3] & 0x3ffffff;

    carry = h[0] >> 26;
    h[1] += carry;
    h[0] &= 0x3ffffff;
    carry = h[1] >> 26;
    h[2] += carry;
    h[1] &= 0x3ffffff;
    carry = h[2] >> 26;
    h[3] += carry;
    h[2] &= 0x3ffffff;
    carry = h[3] >> 26;
    h[0] += carry * 5;
    h[3] &= 0x3ffffff;
    carry = h[0] >> 26;
    h[1] += carry;
    h[0] &= 0x3ffffff;

    uint32_t r0 = ctx->r[0], r1 = ctx->r[1], r2 = ctx->r[2], r3 = ctx->r[3];
    uint32_t h0 = h[0], h1 = h[1], h2 = h[2], h3 = h[3];

    s0 = (uint64_t)h0 * r0 + (uint64_t)h1 * r3 * 5 + (uint64_t)h2 * r2 * 5 + (uint64_t)h3 * r1 * 5;
    s1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * r3 * 5 + (uint64_t)h3 * r2 * 5;
    s2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 + (uint64_t)h3 * r3 * 5;
    s3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 + (uint64_t)h3 * r0;

    h[0] = (uint32_t)(s0 & 0x3ffffff);
    h[1] = (uint32_t)((s0 >> 26) | ((s1 & 0x3ffffff) << 6));
    h[2] = (uint32_t)((s1 >> 20) | ((s2 & 0x3ffffff) << 12));
    h[3] = (uint32_t)((s2 >> 14) | ((s3 & 0x3ffffff) << 18));

    memcpy(ctx->buf, h, sizeof(h));
}

void bank_poly1305_update(bank_poly1305_ctx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len == 16)
        {
            bank_poly1305_add_block(ctx, ctx->buf, 16, 0);
            ctx->buf_len = 0;
        }
    }
    ctx->total_len += len;
}

void bank_poly1305_final(bank_poly1305_ctx *ctx, uint8_t *tag)
{
    if (ctx->buf_len > 0)
    {
        bank_poly1305_add_block(ctx, ctx->buf, ctx->buf_len, 1);
    }
    else
    {
        bank_poly1305_add_block(ctx, NULL, 0, 1);
    }

    uint32_t h[4];
    memcpy(h, ctx->buf, sizeof(h));

    uint32_t tmp;
    h[0] += ctx->s[0];
    h[1] += ctx->s[1];
    h[2] += ctx->s[2];
    h[3] += ctx->s[3];

    tmp = h[0] >> 26;
    h[1] += tmp;
    h[0] &= 0x3ffffff;
    tmp = h[1] >> 26;
    h[2] += tmp;
    h[1] &= 0x3ffffff;
    tmp = h[2] >> 26;
    h[3] += tmp;
    h[2] &= 0x3ffffff;
    tmp = h[3] >> 26;
    h[0] += tmp * 5;
    h[3] &= 0x3ffffff;
    tmp = h[0] >> 26;
    h[1] += tmp;
    h[0] &= 0x3ffffff;

    tag[0] = h[0] & 0xff;
    tag[1] = (h[0] >> 8) & 0xff;
    tag[2] = (h[0] >> 16) & 0xff;
    tag[3] = (h[0] >> 24) & 0xff;
    tag[4] = h[1] & 0xff;
    tag[5] = (h[1] >> 8) & 0xff;
    tag[6] = (h[1] >> 16) & 0xff;
    tag[7] = (h[1] >> 24) & 0xff;
    tag[8] = h[2] & 0xff;
    tag[9] = (h[2] >> 8) & 0xff;
    tag[10] = (h[2] >> 16) & 0xff;
    tag[11] = (h[2] >> 24) & 0xff;
    tag[12] = h[3] & 0xff;
    tag[13] = (h[3] >> 8) & 0xff;
    tag[14] = (h[3] >> 16) & 0xff;
    tag[15] = (h[3] >> 24) & 0xff;
}
