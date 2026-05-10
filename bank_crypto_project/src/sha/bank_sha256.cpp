#include "bank/bank_sha.h"
#include <string.h>

const uint32_t bank_sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static void bank_sha256_transform(bank_sha256_ctx *ctx)
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;

    for (int i = 0; i < 16; i++)
    {
        w[i] = ((uint32_t)ctx->buffer[i * 4] << 24) |
               ((uint32_t)ctx->buffer[i * 4 + 1] << 16) |
               ((uint32_t)ctx->buffer[i * 4 + 2] << 8) |
               ((uint32_t)ctx->buffer[i * 4 + 3]);
    }

    for (int i = 16; i < 64; i++)
    {
        w[i] = bank_ssig1(w[i - 2]) + w[i - 7] + bank_ssig0(w[i - 15]) + w[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (int i = 0; i < 64; i++)
    {
        uint32_t t1 = h + bank_bsig1(e) + bank_ch32(e, f, g) + bank_sha256_k[i] + w[i];
        uint32_t t2 = bank_bsig0(a) + bank_maj32(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void bank_sha256_init(bank_sha256_ctx *ctx)
{
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
    memset(ctx->buffer, 0, BANK_SHA256_BLOCK_SIZE);
}

void bank_sha256_update(bank_sha256_ctx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ctx->buffer[ctx->count % BANK_SHA256_BLOCK_SIZE] = data[i];
        ctx->count++;

        if ((ctx->count % BANK_SHA256_BLOCK_SIZE) == 0)
        {
            bank_sha256_transform(ctx);
        }
    }
}

void bank_sha256_final(bank_sha256_ctx *ctx, uint8_t *hash)
{
    uint64_t bit_len = ctx->count * 8;
    size_t pad_len;
    uint8_t padding[BANK_SHA256_BLOCK_SIZE];

    pad_len = (ctx->count % BANK_SHA256_BLOCK_SIZE) < 56 ? 56 - (ctx->count % BANK_SHA256_BLOCK_SIZE) : 120 - (ctx->count % BANK_SHA256_BLOCK_SIZE);

    memset(padding, 0, pad_len);
    padding[0] = 0x80;
    bank_sha256_update(ctx, padding, pad_len);

    for (int i = 0; i < 8; i++)
    {
        padding[i] = (uint8_t)(bit_len >> (56 - i * 8));
    }
    bank_sha256_update(ctx, padding, 8);

    for (int i = 0; i < 8; i++)
    {
        hash[i * 4] = (uint8_t)(ctx->state[i] >> 24);
        hash[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

void bank_sha256(const uint8_t *data, size_t len, uint8_t *hash)
{
    bank_sha256_ctx ctx;
    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, data, len);
    bank_sha256_final(&ctx, hash);
}

uint32_t bank_rotr32(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}

uint32_t bank_ch32(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

uint32_t bank_maj32(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t bank_bsig0(uint32_t x)
{
    return bank_rotr32(x, 2) ^ bank_rotr32(x, 13) ^ bank_rotr32(x, 22);
}

uint32_t bank_bsig1(uint32_t x)
{
    return bank_rotr32(x, 6) ^ bank_rotr32(x, 11) ^ bank_rotr32(x, 25);
}

uint32_t bank_ssig0(uint32_t x)
{
    return bank_rotr32(x, 7) ^ bank_rotr32(x, 18) ^ (x >> 3);
}

uint32_t bank_ssig1(uint32_t x)
{
    return bank_rotr32(x, 17) ^ bank_rotr32(x, 19) ^ (x >> 10);
}
