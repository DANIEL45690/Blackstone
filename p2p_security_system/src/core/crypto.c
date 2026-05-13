#include "crypto.h"
#include <windows.h>
#include <wincrypt.h>

typedef struct
{
    HCRYPTPROV hProv;
    CRITICAL_SECTION lock;
    int initialized;
} crypto_provider_t;

static crypto_provider_t g_crypto = {0};
static uint32_t crc32_table[256] = {0};
static int crc32_table_initialized = 0;

static void init_crc32_table(void)
{
    if (crc32_table_initialized)
        return;
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

uint32_t crc32(const uint8_t *data, size_t len)
{
    init_crc32_table();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
    {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return ~crc;
}

typedef struct
{
    uint32_t state[8];
    uint8_t buffer[64];
    uint64_t count;
} sha256_ctx_t;

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static void sha256_transform(sha256_ctx_t *ctx)
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;

    for (int i = 0; i < 16; i++)
    {
        w[i] = (ctx->buffer[i * 4] << 24) | (ctx->buffer[i * 4 + 1] << 16) |
               (ctx->buffer[i * 4 + 2] << 8) | ctx->buffer[i * 4 + 3];
    }

    for (int i = 16; i < 64; i++)
    {
        uint32_t s0 = (w[i - 15] >> 7) | (w[i - 15] << 25);
        uint32_t s1 = (w[i - 2] >> 17) | (w[i - 2] << 15);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
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
        uint32_t S1 = (e >> 6) | (e << 26);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + sha256_k[i] + w[i];
        uint32_t S0 = (a >> 2) | (a << 30);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
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

static void sha256_init(sha256_ctx_t *ctx)
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
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ctx->buffer[ctx->count % 64] = data[i];
        ctx->count++;
        if ((ctx->count % 64) == 0)
        {
            sha256_transform(ctx);
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t *hash)
{
    uint64_t bit_len = ctx->count * 8;
    size_t pad_len = (ctx->count % 64) < 56 ? 56 - (ctx->count % 64) : 120 - (ctx->count % 64);
    uint8_t padding[64] = {0x80};

    sha256_update(ctx, padding, pad_len);

    for (int i = 0; i < 8; i++)
    {
        padding[i] = (uint8_t)(bit_len >> (56 - i * 8));
    }
    sha256_update(ctx, padding, 8);

    for (int i = 0; i < 8; i++)
    {
        hash[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = ctx->state[i] & 0xFF;
    }
}

void sha256(const uint8_t *data, size_t len, uint8_t *hash)
{
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

int crypto_init(void)
{
    if (g_crypto.initialized)
        return 1;

    if (!CryptAcquireContextW(&g_crypto.hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        if (!CryptAcquireContextW(&g_crypto.hProv, NULL, NULL, PROV_RSA_AES, CRYPT_NEWKEYSET))
        {
            return 0;
        }
    }

    InitializeCriticalSection(&g_crypto.lock);
    g_crypto.initialized = 1;
    return 1;
}

void crypto_cleanup(void)
{
    if (g_crypto.initialized)
    {
        CryptReleaseContext(g_crypto.hProv, 0);
        DeleteCriticalSection(&g_crypto.lock);
        g_crypto.initialized = 0;
    }
}

int random_bytes(uint8_t *buf, size_t len)
{
    if (!g_crypto.initialized)
    {
        for (size_t i = 0; i < len; i++)
        {
            buf[i] = (uint8_t)(rand() % 256);
        }
        return 1;
    }

    EnterCriticalSection(&g_crypto.lock);
    int result = CryptGenRandom(g_crypto.hProv, (DWORD)len, buf);
    LeaveCriticalSection(&g_crypto.lock);
    return result;
}

void simple_xor_encrypt(uint8_t *data, size_t len, const uint8_t *key, size_t key_len)
{
    for (size_t i = 0; i < len; i++)
    {
        data[i] ^= key[i % key_len];
    }
}

void simple_xor_decrypt(uint8_t *data, size_t len, const uint8_t *key, size_t key_len)
{
    for (size_t i = 0; i < len; i++)
    {
        data[i] ^= key[i % key_len];
    }
}

int secure_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    volatile uint8_t result = 0;
    for (size_t i = 0; i < len; i++)
    {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}

void secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--)
        *p++ = 0;
}
