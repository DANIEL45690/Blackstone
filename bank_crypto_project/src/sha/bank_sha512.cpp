#include "bank/bank_sha.h"
#include <string.h>

const uint64_t bank_sha512_k[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL};

static void bank_sha512_transform(bank_sha512_ctx *ctx)
{
    uint64_t w[80];
    uint64_t a, b, c, d, e, f, g, h;

    for (int i = 0; i < 16; i++)
    {
        w[i] = ((uint64_t)ctx->buffer[i * 8] << 56) |
               ((uint64_t)ctx->buffer[i * 8 + 1] << 48) |
               ((uint64_t)ctx->buffer[i * 8 + 2] << 40) |
               ((uint64_t)ctx->buffer[i * 8 + 3] << 32) |
               ((uint64_t)ctx->buffer[i * 8 + 4] << 24) |
               ((uint64_t)ctx->buffer[i * 8 + 5] << 16) |
               ((uint64_t)ctx->buffer[i * 8 + 6] << 8) |
               ((uint64_t)ctx->buffer[i * 8 + 7]);
    }

    for (int i = 16; i < 80; i++)
    {
        w[i] = bank_ssig1_64(w[i - 2]) + w[i - 7] + bank_ssig0_64(w[i - 15]) + w[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (int i = 0; i < 80; i++)
    {
        uint64_t t1 = h + bank_bsig1_64(e) + bank_ch64(e, f, g) + bank_sha512_k[i] + w[i];
        uint64_t t2 = bank_bsig0_64(a) + bank_maj64(a, b, c);
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

void bank_sha512_init(bank_sha512_ctx *ctx)
{
    ctx->state[0] = 0x6a09e667f3bcc908ULL;
    ctx->state[1] = 0xbb67ae8584caa73bULL;
    ctx->state[2] = 0x3c6ef372fe94f82bULL;
    ctx->state[3] = 0xa54ff53a5f1d36f1ULL;
    ctx->state[4] = 0x510e527fade682d1ULL;
    ctx->state[5] = 0x9b05688c2b3e6c1fULL;
    ctx->state[6] = 0x1f83d9abfb41bd6bULL;
    ctx->state[7] = 0x5be0cd19137e2179ULL;
    ctx->count = 0;
    memset(ctx->buffer, 0, BANK_SHA512_BLOCK_SIZE);
}

void bank_sha512_update(bank_sha512_ctx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ctx->buffer[ctx->count % BANK_SHA512_BLOCK_SIZE] = data[i];
        ctx->count++;

        if ((ctx->count % BANK_SHA512_BLOCK_SIZE) == 0)
        {
            bank_sha512_transform(ctx);
        }
    }
}

void bank_sha512_final(bank_sha512_ctx *ctx, uint8_t *hash)
{
    uint64_t bit_len = ctx->count * 8;
    size_t pad_len;
    uint8_t padding[BANK_SHA512_BLOCK_SIZE];

    if ((ctx->count % BANK_SHA512_BLOCK_SIZE) < 112)
        pad_len = 112 - (ctx->count % BANK_SHA512_BLOCK_SIZE);
    else
        pad_len = 120 - (ctx->count % BANK_SHA512_BLOCK_SIZE);

    memset(padding, 0, pad_len);
    padding[0] = 0x80;
    bank_sha512_update(ctx, padding, pad_len);

    for (int i = 0; i < 8; i++)
    {
        padding[i] = (uint8_t)(bit_len >> (56 - i * 8));
    }
    bank_sha512_update(ctx, padding, 8);

    for (int i = 0; i < 8; i++)
    {
        hash[i * 8] = (uint8_t)(ctx->state[i] >> 56);
        hash[i * 8 + 1] = (uint8_t)(ctx->state[i] >> 48);
        hash[i * 8 + 2] = (uint8_t)(ctx->state[i] >> 40);
        hash[i * 8 + 3] = (uint8_t)(ctx->state[i] >> 32);
        hash[i * 8 + 4] = (uint8_t)(ctx->state[i] >> 24);
        hash[i * 8 + 5] = (uint8_t)(ctx->state[i] >> 16);
        hash[i * 8 + 6] = (uint8_t)(ctx->state[i] >> 8);
        hash[i * 8 + 7] = (uint8_t)(ctx->state[i]);
    }
}

void bank_sha512(const uint8_t *data, size_t len, uint8_t *hash)
{
    bank_sha512_ctx ctx;
    bank_sha512_init(&ctx);
    bank_sha512_update(&ctx, data, len);
    bank_sha512_final(&ctx, hash);
}

uint64_t bank_rotr64(uint64_t x, int n)
{
    return (x >> n) | (x << (64 - n));
}

uint64_t bank_ch64(uint64_t x, uint64_t y, uint64_t z)
{
    return (x & y) ^ (~x & z);
}

uint64_t bank_maj64(uint64_t x, uint64_t y, uint64_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

uint64_t bank_bsig0_64(uint64_t x)
{
    return bank_rotr64(x, 28) ^ bank_rotr64(x, 34) ^ bank_rotr64(x, 39);
}

uint64_t bank_bsig1_64(uint64_t x)
{
    return bank_rotr64(x, 14) ^ bank_rotr64(x, 18) ^ bank_rotr64(x, 41);
}

uint64_t bank_ssig0_64(uint64_t x)
{
    return bank_rotr64(x, 1) ^ bank_rotr64(x, 8) ^ (x >> 7);
}

uint64_t bank_ssig1_64(uint64_t x)
{
    return bank_rotr64(x, 19) ^ bank_rotr64(x, 61) ^ (x >> 6);
}
