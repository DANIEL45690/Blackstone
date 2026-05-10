#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <wincrypt.h>
#include <synchapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#endif

#define BANK_AES_BLOCK_SIZE 16
#define BANK_AES_KEY_BYTES 32
#define BANK_AES_KEY_BITS 256
#define BANK_GCM_IV_BYTES 12
#define BANK_GCM_TAG_BYTES 16
#define BANK_SHA256_BLOCK_SIZE 64
#define BANK_SHA256_DIGEST_SIZE 32
#define BANK_HMAC_BLOCK_SIZE 64
#define BANK_MAX_DATA (1024 * 1024 * 10)
#define BANK_PBKDF2_ROUNDS 100000
#define BANK_MAX_KEY_ID 256
#define BANK_MAX_SESSION_KEYS 32
#define BANK_LOG_BUFFER_SIZE 4096
#define BANK_MAX_AAD_SIZE 65536
#define BANK_SALSA20_BLOCK_SIZE 64
#define BANK_POLY1305_KEY_SIZE 32
#define BANK_POLY1305_TAG_SIZE 16
#define BANK_CHACHA20_BLOCK_SIZE 64
#define BANK_RNG_POOL_SIZE 1024
#define BANK_MAX_KEY_ROTATIONS 8
#define BANK_HKDF_EXTRACT_SIZE 32
#define BANK_SECURE_COMPARE_DELAY_MS 10

typedef enum
{
    BANK_LOG_DEBUG = 0,
    BANK_LOG_INFO = 1,
    BANK_LOG_WARN = 2,
    BANK_LOG_ERROR = 3,
    BANK_LOG_FATAL = 4
} bank_log_level;

typedef struct
{
    uint32_t state[8];
    uint8_t buffer[BANK_SHA256_BLOCK_SIZE];
    uint64_t count;
} bank_sha256_ctx;

typedef struct
{
    uint64_t state[8];
    uint8_t buffer[128];
    uint64_t count;
} bank_sha512_ctx;
typedef struct
{
    uint32_t key[60];
    int rounds;
} bank_aes_key_schedule;

typedef struct
{
    bank_aes_key_schedule enc_key;
    bank_aes_key_schedule dec_key;
} bank_aes256_key;

typedef struct
{
    uint8_t key[BANK_AES_KEY_BYTES];
    uint8_t nonce[12];
    uint32_t counter;
} bank_chacha20_ctx;

typedef struct
{
    uint8_t key[BANK_POLY1305_KEY_SIZE];
    uint32_t r[4];
    uint32_t s[4];
    uint8_t buf[16];
    size_t buf_len;
    uint64_t total_len;
} bank_poly1305_ctx;

typedef struct
{
    uint8_t master_key[BANK_AES_KEY_BYTES];
    uint8_t salt[32];
    uint8_t anti_tamper_seed[32];
    uint8_t key_rotation_seed[32];
    int initialized;
    int ref_count;
    uint64_t total_encryptions;
    uint64_t total_decryptions;
    time_t key_created_at;
    uint8_t key_chain[BANK_MAX_KEY_ROTATIONS][BANK_AES_KEY_BYTES];
    int current_key_index;
#if defined(_WIN32) || defined(_WIN64)
    CRITICAL_SECTION mutex;
    CRITICAL_SECTION rng_mutex;
#else
    pthread_mutex_t mutex;
    pthread_mutex_t rng_mutex;
#endif
} bank_crypto_context;

typedef struct
{
    uint8_t session_key[BANK_AES_KEY_BYTES];
    uint8_t hmac_key[BANK_AES_KEY_BYTES];
    uint8_t key_id[BANK_MAX_KEY_ID];
    size_t key_id_len;
    time_t created_at;
    time_t expires_at;
    time_t last_used_at;
    uint64_t use_count;
    int active;
    uint8_t salt[16];
} bank_session_key;

typedef struct
{
    bank_session_key keys[BANK_MAX_SESSION_KEYS];
    int count;
    uint64_t total_sessions_created;
    uint64_t total_sessions_destroyed;
#if defined(_WIN32) || defined(_WIN64)
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
} bank_session_manager;

typedef struct
{
    uint64_t transaction_id;
    uint64_t timestamp;
    uint8_t from_account[32];
    uint8_t to_account[32];
    uint64_t amount;
    uint8_t currency[8];
    uint8_t signature[BANK_SHA256_DIGEST_SIZE];
    int verified;
    uint64_t nonce;
    uint8_t reference_hash[32];
    uint8_t metadata[128];
    size_t metadata_len;
} bank_transaction;

typedef struct
{
    uint8_t encrypted_data[BANK_MAX_DATA];
    size_t encrypted_len;
    uint8_t iv[BANK_GCM_IV_BYTES];
    uint8_t tag[BANK_GCM_TAG_BYTES];
    uint8_t key_id[BANK_MAX_KEY_ID];
    size_t key_id_len;
    uint64_t timestamp;
    uint32_t crc32;
    uint32_t version;
    uint8_t auth_tag[32];
    uint8_t compression_flag;
} bank_secure_envelope;

typedef struct
{
    uint8_t pool[BANK_RNG_POOL_SIZE];
    size_t pool_index;
    uint64_t entropy_bits;
} bank_rng_state;

static bank_log_level g_log_level = BANK_LOG_INFO;
static FILE *g_log_file = NULL;
static bank_crypto_context g_ctx = {0};
static bank_session_manager g_session_mgr = {0};
static bank_rng_state g_rng_state = {0};
static int g_library_initialized = 0;
static uint64_t g_library_start_time = 0;

static const uint32_t bank_sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static const uint64_t bank_sha512_k[80] = {
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

static const uint8_t bank_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

static uint32_t bank_rotr32(uint32_t x, int n)
{
    return (x >> n) | (x << (32 - n));
}

static uint64_t bank_rotr64(uint64_t x, int n)
{
    return (x >> n) | (x << (64 - n));
}

static uint32_t bank_ch32(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static uint32_t bank_maj32(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t bank_bsig0(uint32_t x)
{
    return bank_rotr32(x, 2) ^ bank_rotr32(x, 13) ^ bank_rotr32(x, 22);
}

static uint32_t bank_bsig1(uint32_t x)
{
    return bank_rotr32(x, 6) ^ bank_rotr32(x, 11) ^ bank_rotr32(x, 25);
}

static uint32_t bank_ssig0(uint32_t x)
{
    return bank_rotr32(x, 7) ^ bank_rotr32(x, 18) ^ (x >> 3);
}

static uint32_t bank_ssig1(uint32_t x)
{
    return bank_rotr32(x, 17) ^ bank_rotr32(x, 19) ^ (x >> 10);
}

static uint64_t bank_ch64(uint64_t x, uint64_t y, uint64_t z)
{
    return (x & y) ^ (~x & z);
}

static uint64_t bank_maj64(uint64_t x, uint64_t y, uint64_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint64_t bank_bsig0_64(uint64_t x)
{
    return bank_rotr64(x, 28) ^ bank_rotr64(x, 34) ^ bank_rotr64(x, 39);
}

static uint64_t bank_bsig1_64(uint64_t x)
{
    return bank_rotr64(x, 14) ^ bank_rotr64(x, 18) ^ bank_rotr64(x, 41);
}

static uint64_t bank_ssig0_64(uint64_t x)
{
    return bank_rotr64(x, 1) ^ bank_rotr64(x, 8) ^ (x >> 7);
}

static uint64_t bank_ssig1_64(uint64_t x)
{
    return bank_rotr64(x, 19) ^ bank_rotr64(x, 61) ^ (x >> 6);
}

void bank_log(bank_log_level level, const char *file, int line, const char *fmt, ...)
{
    if (level < g_log_level)
        return;

    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    char buffer[BANK_LOG_BUFFER_SIZE];
    va_list args;
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);

    va_start(args, fmt);
    vsnprintf(buffer, BANK_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    if (g_log_file)
    {
        fprintf(g_log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s:%d] %s\n",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                level_str[level], file, line, buffer);
        fflush(g_log_file);
    }
    else
    {
        fprintf(stderr, "[%s] %s\n", level_str[level], buffer);
    }
}

#define BANK_LOG(level, ...) bank_log(level, __FILE__, __LINE__, __VA_ARGS__)

void bank_set_log_level(bank_log_level level)
{
    g_log_level = level;
}

int bank_set_log_file(const char *filename)
{
    if (g_log_file)
        fclose(g_log_file);
    g_log_file = fopen(filename, "a");
    if (!g_log_file)
        return 0;
    setvbuf(g_log_file, NULL, _IONBF, 0);
    return 1;
}

void bank_close_log_file(void)
{
    if (g_log_file)
        fclose(g_log_file);
    g_log_file = NULL;
}

void bank_secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--)
        *p++ = 0;
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
    memset(ctx->buffer, 0, BANK_SHA256_BLOCK_SIZE); // ← 64 байта, не 128!
}
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
    memset(ctx->buffer, 0, 128); // ← 128 байт для SHA-512
}

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

void bank_sha512_update(bank_sha512_ctx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        ctx->buffer[ctx->count % BANK_SHA256_BLOCK_SIZE] = data[i];
        ctx->count++;

        if ((ctx->count % BANK_SHA256_BLOCK_SIZE) == 0)
        {
            bank_sha512_transform(ctx);
        }
    }
}

void bank_sha512_final(bank_sha512_ctx *ctx, uint8_t *hash)
{
    uint64_t bit_len = ctx->count * 8;
    size_t pad_len;
    uint8_t padding[BANK_SHA256_BLOCK_SIZE];

    if ((ctx->count % 128) < 112)
    {
        pad_len = 112 - (ctx->count % 128);
    }
    else
    {
        pad_len = 120 - (ctx->count % 128);
    }

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

static uint8_t bank_xtime(uint8_t x)
{
    return (x << 1) ^ ((x & 0x80) ? 0x1b : 0);
}

static void bank_aes_shift_rows(uint8_t *state)
{
    uint8_t t;
    t = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = t;
    t = state[2];
    state[2] = state[10];
    state[10] = t;
    t = state[6];
    state[6] = state[14];
    state[14] = t;
    t = state[3];
    state[3] = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = t;
}

static void bank_aes_sub_bytes(uint8_t *state)
{
    for (int i = 0; i < 16; i++)
        state[i] = bank_sbox[state[i]];
}

static void bank_aes_mix_columns(uint8_t *state)
{
    uint8_t a, b, c, d, t;
    for (int i = 0; i < 4; i++)
    {
        a = state[i * 4];
        b = state[i * 4 + 1];
        c = state[i * 4 + 2];
        d = state[i * 4 + 3];
        t = a ^ b ^ c ^ d;
        state[i * 4] = a ^ b ^ bank_xtime(a ^ b) ^ t;
        state[i * 4 + 1] = b ^ c ^ bank_xtime(b ^ c) ^ t;
        state[i * 4 + 2] = c ^ d ^ bank_xtime(c ^ d) ^ t;
        state[i * 4 + 3] = d ^ a ^ bank_xtime(d ^ a) ^ t;
    }
}

static void bank_aes_add_round_key(uint8_t *state, const uint32_t *key, int round)
{
    for (int i = 0; i < 4; i++)
    {
        state[i * 4] ^= (key[round * 4 + i] >> 24) & 0xFF;
        state[i * 4 + 1] ^= (key[round * 4 + i] >> 16) & 0xFF;
        state[i * 4 + 2] ^= (key[round * 4 + i] >> 8) & 0xFF;
        state[i * 4 + 3] ^= (key[round * 4 + i]) & 0xFF;
    }
}

static void bank_aes_key_expansion(const uint8_t *key, uint32_t *round_keys, int key_bits)
{
    int nk = key_bits / 32;
    int nr = nk + 6;
    uint32_t temp;

    for (int i = 0; i < nk; i++)
    {
        round_keys[i] = ((uint32_t)key[i * 4] << 24) |
                        ((uint32_t)key[i * 4 + 1] << 16) |
                        ((uint32_t)key[i * 4 + 2] << 8) |
                        ((uint32_t)key[i * 4 + 3]);
    }

    for (int i = nk; i < (nr + 1) * 4; i++)
    {
        temp = round_keys[i - 1];
        if (i % nk == 0)
        {
            temp = ((temp << 8) | (temp >> 24));
            temp = (bank_sbox[temp >> 24] << 24) |
                   (bank_sbox[(temp >> 16) & 0xFF] << 16) |
                   (bank_sbox[(temp >> 8) & 0xFF] << 8) |
                   (bank_sbox[temp & 0xFF]);
            temp ^= (bank_sha256_k[(i / nk) - 1] << 24);
        }
        else if (nk > 6 && i % nk == 4)
        {
            temp = (bank_sbox[temp >> 24] << 24) |
                   (bank_sbox[(temp >> 16) & 0xFF] << 16) |
                   (bank_sbox[(temp >> 8) & 0xFF] << 8) |
                   (bank_sbox[temp & 0xFF]);
        }
        round_keys[i] = round_keys[i - nk] ^ temp;
    }
}

static void bank_aes_encrypt_block(const uint8_t *input, uint8_t *output, const bank_aes256_key *key)
{
    uint8_t state[16];
    memcpy(state, input, 16);

    bank_aes_add_round_key(state, key->enc_key.key, 0);

    for (int round = 1; round <= 13; round++)
    {
        bank_aes_sub_bytes(state);
        bank_aes_shift_rows(state);
        bank_aes_mix_columns(state);
        bank_aes_add_round_key(state, key->enc_key.key, round);
    }

    bank_aes_sub_bytes(state);
    bank_aes_shift_rows(state);
    bank_aes_add_round_key(state, key->enc_key.key, 14);

    memcpy(output, state, 16);
}

static void bank_aes256_set_key(bank_aes256_key *key, const uint8_t *user_key)
{
    uint32_t enc_round_keys[60] = {0};
    uint32_t dec_round_keys[60] = {0};

    bank_aes_key_expansion(user_key, enc_round_keys, BANK_AES_KEY_BITS);

    for (int i = 0; i <= 14; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            dec_round_keys[(14 - i) * 4 + j] = enc_round_keys[i * 4 + j];
        }
    }

    key->enc_key.rounds = 14;
    key->dec_key.rounds = 14;
    memcpy(key->enc_key.key, enc_round_keys, sizeof(enc_round_keys));
    memcpy(key->dec_key.key, dec_round_keys, sizeof(dec_round_keys));
}

static void bank_gctr(const bank_aes256_key *key, const uint8_t *icb, const uint8_t *x, size_t xlen, uint8_t *y)
{
    uint8_t cb[16];
    uint8_t enc_cb[16];
    size_t n_blocks = (xlen + 15) / 16;
    size_t last_block_len = xlen % 16;

    memcpy(cb, icb, 16);

    for (size_t i = 0; i < n_blocks; i++)
    {
        cb[15]++;
        if (cb[15] == 0)
        {
            for (int j = 14; j >= 0; j--)
            {
                cb[j]++;
                if (cb[j] != 0)
                    break;
            }
        }

        bank_aes_encrypt_block(cb, enc_cb, key);

        size_t block_len = (i == n_blocks - 1 && last_block_len > 0) ? last_block_len : 16;

        for (size_t j = 0; j < block_len; j++)
        {
            y[i * 16 + j] = x[i * 16 + j] ^ enc_cb[j];
        }
    }
}

static void bank_ghash(uint8_t *h, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t cipher_len, uint8_t *s)
{
    uint8_t x[16] = {0};
    uint8_t temp[16];

    size_t aad_blocks = (aad_len + 15) / 16;
    size_t cipher_blocks = (cipher_len + 15) / 16;

    for (size_t i = 0; i < aad_blocks; i++)
    {
        memset(temp, 0, 16);
        size_t block_len = (i == aad_blocks - 1 && aad_len % 16 != 0) ? aad_len % 16 : 16;
        memcpy(temp, aad + i * 16, block_len);

        for (size_t j = 0; j < 16; j++)
            x[j] ^= temp[j];

        for (size_t j = 0; j < 16; j++)
        {
            uint8_t z = x[15];
            for (int k = 15; k >= 0; k--)
            {
                x[k] = (x[k] >> 1) ^ ((k > 0 ? (x[k - 1] & 1) : 0) << 7);
                if (z & 1)
                    x[k] ^= h[k];
                z >>= 1;
            }
        }
    }

    for (size_t i = 0; i < cipher_blocks; i++)
    {
        memset(temp, 0, 16);
        size_t block_len = (i == cipher_blocks - 1 && cipher_len % 16 != 0) ? cipher_len % 16 : 16;
        memcpy(temp, ciphertext + i * 16, block_len);

        for (size_t j = 0; j < 16; j++)
            x[j] ^= temp[j];

        for (size_t j = 0; j < 16; j++)
        {
            uint8_t z = x[15];
            for (int k = 15; k >= 0; k--)
            {
                x[k] = (x[k] >> 1) ^ ((k > 0 ? (x[k - 1] & 1) : 0) << 7);
                if (z & 1)
                    x[k] ^= h[k];
                z >>= 1;
            }
        }
    }

    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t cipher_bits = cipher_len * 8;
    for (int i = 0; i < 8; i++)
    {
        len_block[i] = (aad_bits >> (56 - i * 8)) & 0xFF;
        len_block[8 + i] = (cipher_bits >> (56 - i * 8)) & 0xFF;
    }

    for (size_t i = 0; i < 16; i++)
        x[i] ^= len_block[i];

    for (size_t i = 0; i < 16; i++)
    {
        uint8_t z = x[15];
        for (int k = 15; k >= 0; k--)
        {
            x[k] = (x[k] >> 1) ^ ((k > 0 ? (x[k - 1] & 1) : 0) << 7);
            if (z & 1)
                x[k] ^= h[k];
            z >>= 1;
        }
    }

    memcpy(s, x, 16);
}

int bank_aes256_gcm_encrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *plain, size_t plain_len,
                            uint8_t *cipher, uint8_t *tag)
{
    bank_aes256_key aes_key;
    uint8_t h[16];
    uint8_t j0[16];
    uint8_t zero_block[16] = {0};

    if (iv_len != BANK_GCM_IV_BYTES)
        return 0;
    if (!key || !iv || !plain || !cipher || !tag)
        return 0;

    bank_aes256_set_key(&aes_key, key);
    bank_aes_encrypt_block(zero_block, h, &aes_key);

    memset(j0, 0, 16);
    memcpy(j0, iv, iv_len);
    j0[15] = 1;

    bank_gctr(&aes_key, j0, plain, plain_len, cipher);

    uint8_t s[16];
    bank_ghash(h, aad, aad_len, cipher, plain_len, s);

    uint8_t enc_s[16];
    bank_aes_encrypt_block(j0, enc_s, &aes_key);

    for (int i = 0; i < 16; i++)
        tag[i] = s[i] ^ enc_s[i];

    bank_secure_zero(&aes_key, sizeof(aes_key));
    bank_secure_zero(h, sizeof(h));
    bank_secure_zero(j0, sizeof(j0));
    bank_secure_zero(s, sizeof(s));
    bank_secure_zero(enc_s, sizeof(enc_s));

    return 1;
}

int bank_aes256_gcm_decrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *cipher, size_t cipher_len,
                            const uint8_t *tag, uint8_t *plain)
{
    bank_aes256_key aes_key;
    uint8_t h[16];
    uint8_t j0[16];
    uint8_t zero_block[16] = {0};
    uint8_t s[16];
    uint8_t enc_s[16];
    uint8_t expected_tag[16];

    if (iv_len != BANK_GCM_IV_BYTES)
        return 0;
    if (!key || !iv || !cipher || !tag || !plain)
        return 0;

    bank_aes256_set_key(&aes_key, key);
    bank_aes_encrypt_block(zero_block, h, &aes_key);

    memset(j0, 0, 16);
    memcpy(j0, iv, iv_len);
    j0[15] = 1;

    bank_ghash(h, aad, aad_len, cipher, cipher_len, s);
    bank_aes_encrypt_block(j0, enc_s, &aes_key);

    for (int i = 0; i < 16; i++)
        expected_tag[i] = s[i] ^ enc_s[i];

    int tag_ok = 1;
    for (int i = 0; i < 16; i++)
    {
        if (tag[i] != expected_tag[i])
            tag_ok = 0;
    }

    if (tag_ok)
    {
        bank_gctr(&aes_key, j0, cipher, cipher_len, plain);
    }

    bank_secure_zero(&aes_key, sizeof(aes_key));
    bank_secure_zero(h, sizeof(h));
    bank_secure_zero(j0, sizeof(j0));
    bank_secure_zero(s, sizeof(s));
    bank_secure_zero(enc_s, sizeof(enc_s));
    bank_secure_zero(expected_tag, sizeof(expected_tag));

    return tag_ok;
}

static void bank_chacha20_quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    *a += *b;
    *d ^= *a;
    *d = (*d << 16) | (*d >> 16);
    *c += *d;
    *b ^= *c;
    *b = (*b << 12) | (*b >> 20);
    *a += *b;
    *d ^= *a;
    *d = (*d << 8) | (*d >> 24);
    *c += *d;
    *b ^= *c;
    *b = (*b << 7) | (*b >> 25);
}

static void bank_chacha20_block(bank_chacha20_ctx *ctx, uint8_t *output)
{
    uint32_t x[16];
    for (int i = 0; i < 16; i++)
    {
        if (i < 8)
            x[i] = ((uint32_t *)ctx->key)[i];
        else if (i < 12)
            x[i] = ((uint32_t *)ctx->nonce)[i - 8];
        else
            x[i] = ctx->counter;
    }

    uint32_t working[16];
    memcpy(working, x, sizeof(working));

    for (int i = 0; i < 10; i++)
    {
        bank_chacha20_quarter_round(&working[0], &working[4], &working[8], &working[12]);
        bank_chacha20_quarter_round(&working[1], &working[5], &working[9], &working[13]);
        bank_chacha20_quarter_round(&working[2], &working[6], &working[10], &working[14]);
        bank_chacha20_quarter_round(&working[3], &working[7], &working[11], &working[15]);
        bank_chacha20_quarter_round(&working[0], &working[5], &working[10], &working[15]);
        bank_chacha20_quarter_round(&working[1], &working[6], &working[11], &working[12]);
        bank_chacha20_quarter_round(&working[2], &working[7], &working[8], &working[13]);
        bank_chacha20_quarter_round(&working[3], &working[4], &working[9], &working[14]);
    }

    for (int i = 0; i < 16; i++)
    {
        working[i] += x[i];
        output[i * 4] = working[i] & 0xFF;
        output[i * 4 + 1] = (working[i] >> 8) & 0xFF;
        output[i * 4 + 2] = (working[i] >> 16) & 0xFF;
        output[i * 4 + 3] = (working[i] >> 24) & 0xFF;
    }

    ctx->counter++;
}

void bank_chacha20_encrypt(bank_chacha20_ctx *ctx, const uint8_t *input, size_t len, uint8_t *output)
{
    uint8_t block[BANK_CHACHA20_BLOCK_SIZE];
    size_t offset = 0;

    while (len > 0)
    {
        bank_chacha20_block(ctx, block);
        size_t block_len = len < BANK_CHACHA20_BLOCK_SIZE ? len : BANK_CHACHA20_BLOCK_SIZE;

        for (size_t i = 0; i < block_len; i++)
        {
            output[offset + i] = input[offset + i] ^ block[i];
        }

        len -= block_len;
        offset += block_len;
    }
}

static void bank_poly1305_init(bank_poly1305_ctx *ctx, const uint8_t *key)
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

    uint64_t s0 = (uint64_t)h0 * r0 + (uint64_t)h1 * r3 * 5 + (uint64_t)h2 * r2 * 5 + (uint64_t)h3 * r1 * 5;
    uint64_t s1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * r3 * 5 + (uint64_t)h3 * r2 * 5;
    uint64_t s2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 + (uint64_t)h3 * r3 * 5;
    uint64_t s3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 + (uint64_t)h3 * r0;

    h[0] = (uint32_t)(s0 & 0x3ffffff);
    h[1] = (uint32_t)((s0 >> 26) | ((s1 & 0x3ffffff) << 6));
    h[2] = (uint32_t)((s1 >> 20) | ((s2 & 0x3ffffff) << 12));
    h[3] = (uint32_t)((s2 >> 14) | ((s3 & 0x3ffffff) << 18));

    memcpy(ctx->buf, h, sizeof(h));
}

void bank_poly1305_update(bank_poly1305_ctx *ctx, const uint8_t *data, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
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

int bank_random_bytes(uint8_t *buf, size_t len)
{
    if (!buf || len == 0)
        return 0;

#if defined(_WIN32) || defined(_WIN64)
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return 0;
    }
    if (!CryptGenRandom(hProv, len, buf))
    {
        CryptReleaseContext(hProv, 0);
        return 0;
    }
    CryptReleaseContext(hProv, 0);
    return 1;
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f)
        return 0;
    size_t rd = fread(buf, 1, len, f);
    fclose(f);
    return (rd == len) ? 1 : 0;
#endif
}

static void bank_rng_init(void)
{
    bank_random_bytes(g_rng_state.pool, BANK_RNG_POOL_SIZE);
    g_rng_state.pool_index = 0;
    g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
}

static int bank_rng_bytes(uint8_t *buf, size_t len)
{
    if (!g_library_initialized)
        return 0;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_ctx.rng_mutex);
#else
    pthread_mutex_lock(&g_ctx.rng_mutex);
#endif

    if (g_rng_state.entropy_bits < len * 8)
    {
        bank_random_bytes(g_rng_state.pool, BANK_RNG_POOL_SIZE);
        g_rng_state.entropy_bits = BANK_RNG_POOL_SIZE * 8;
    }

    size_t remaining = len;
    size_t offset = 0;

    while (remaining > 0)
    {
        size_t to_copy = remaining;
        if (to_copy > BANK_RNG_POOL_SIZE - g_rng_state.pool_index)
        {
            to_copy = BANK_RNG_POOL_SIZE - g_rng_state.pool_index;
        }

        memcpy(buf + offset, g_rng_state.pool + g_rng_state.pool_index, to_copy);
        g_rng_state.pool_index += to_copy;
        remaining -= to_copy;
        offset += to_copy;

        if (g_rng_state.pool_index >= BANK_RNG_POOL_SIZE)
        {
            uint8_t new_pool[BANK_RNG_POOL_SIZE];
            bank_sha256(g_rng_state.pool, BANK_RNG_POOL_SIZE, new_pool);
            memcpy(g_rng_state.pool, new_pool, BANK_RNG_POOL_SIZE);
            bank_secure_zero(new_pool, sizeof(new_pool));
            g_rng_state.pool_index = 0;
            g_rng_state.entropy_bits = BANK_SHA256_DIGEST_SIZE * 8;
        }
    }

    g_rng_state.entropy_bits -= len * 8;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_ctx.rng_mutex);
#else
    pthread_mutex_unlock(&g_ctx.rng_mutex);
#endif

    return 1;
}

void bank_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac)
{
    uint8_t ipad[BANK_HMAC_BLOCK_SIZE];
    uint8_t opad[BANK_HMAC_BLOCK_SIZE];
    uint8_t inner_hash[BANK_SHA256_DIGEST_SIZE];
    bank_sha256_ctx ctx;

    memset(ipad, 0, BANK_HMAC_BLOCK_SIZE);
    memset(opad, 0, BANK_HMAC_BLOCK_SIZE);

    if (key_len > BANK_HMAC_BLOCK_SIZE)
    {
        bank_sha256(key, key_len, ipad);
        memcpy(opad, ipad, BANK_SHA256_DIGEST_SIZE);
    }
    else
    {
        memcpy(ipad, key, key_len);
        memcpy(opad, key, key_len);
    }

    for (size_t i = 0; i < BANK_HMAC_BLOCK_SIZE; i++)
    {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }

    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, ipad, BANK_HMAC_BLOCK_SIZE);
    bank_sha256_update(&ctx, data, data_len);
    bank_sha256_final(&ctx, inner_hash);

    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, opad, BANK_HMAC_BLOCK_SIZE);
    bank_sha256_update(&ctx, inner_hash, BANK_SHA256_DIGEST_SIZE);
    bank_sha256_final(&ctx, mac);
}

void bank_hkdf_extract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk)
{
    if (salt == NULL || salt_len == 0)
    {
        uint8_t zeros[BANK_HMAC_BLOCK_SIZE] = {0};
        bank_hmac_sha256(zeros, BANK_HMAC_BLOCK_SIZE, ikm, ikm_len, prk);
    }
    else
    {
        bank_hmac_sha256(salt, salt_len, ikm, ikm_len, prk);
    }
}

void bank_hkdf_expand(const uint8_t *prk, size_t prk_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len)
{
    (void)prk;     // - добавляем эту строку
    (void)prk_len; // - добавляем эту строку

    uint8_t previous[BANK_SHA256_DIGEST_SIZE];
    uint8_t counter = 1;
    size_t produced = 0;

    while (produced < okm_len)
    {
        bank_sha256_ctx ctx;
        bank_sha256_init(&ctx);
        if (produced > 0)
        {
            bank_sha256_update(&ctx, previous, BANK_SHA256_DIGEST_SIZE);
        }
        bank_sha256_update(&ctx, info, info_len);
        bank_sha256_update(&ctx, &counter, 1);
        bank_sha256_final(&ctx, previous);

        size_t to_copy = okm_len - produced;
        if (to_copy > BANK_SHA256_DIGEST_SIZE)
            to_copy = BANK_SHA256_DIGEST_SIZE;
        memcpy(okm + produced, previous, to_copy);
        produced += to_copy;
        counter++;
    }
}

void bank_pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len,
                             const uint8_t *salt, size_t salt_len,
                             int iterations, uint8_t *key, size_t key_len)
{
    uint8_t digest[BANK_SHA256_DIGEST_SIZE];
    uint8_t salt_block[BANK_SHA256_BLOCK_SIZE + 4];
    uint8_t u[BANK_SHA256_DIGEST_SIZE];
    size_t key_idx = 0;
    int block = 1;

    while (key_idx < key_len)
    {
        memcpy(salt_block, salt, salt_len);
        salt_block[salt_len] = (block >> 24) & 0xFF;
        salt_block[salt_len + 1] = (block >> 16) & 0xFF;
        salt_block[salt_len + 2] = (block >> 8) & 0xFF;
        salt_block[salt_len + 3] = block & 0xFF;

        bank_hmac_sha256(password, password_len, salt_block, salt_len + 4, digest);
        memcpy(u, digest, BANK_SHA256_DIGEST_SIZE);

        for (int i = 1; i < iterations; i++)
        {
            bank_hmac_sha256(password, password_len, u, BANK_SHA256_DIGEST_SIZE, u);
            for (int j = 0; j < BANK_SHA256_DIGEST_SIZE; j++)
            {
                digest[j] ^= u[j];
            }
        }

        for (int i = 0; i < BANK_SHA256_DIGEST_SIZE && key_idx < key_len; i++)
        {
            key[key_idx++] = digest[i];
        }

        block++;
    }
}

int bank_init_master_random(void)
{
    if (!bank_random_bytes(g_ctx.master_key, BANK_AES_KEY_BYTES))
        return 0;
    if (!bank_random_bytes(g_ctx.salt, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }
    if (!bank_random_bytes(g_ctx.anti_tamper_seed, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }
    if (!bank_random_bytes(g_ctx.key_rotation_seed, 32))
    {
        bank_secure_zero(g_ctx.master_key, BANK_AES_KEY_BYTES);
        return 0;
    }

#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&g_ctx.mutex);
    InitializeCriticalSection(&g_ctx.rng_mutex);
    InitializeCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_init(&g_ctx.mutex, NULL);
    pthread_mutex_init(&g_ctx.rng_mutex, NULL);
    pthread_mutex_init(&g_session_mgr.mutex, NULL);
#endif

    memcpy(g_ctx.key_chain[0], g_ctx.master_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = 0;
    g_ctx.key_created_at = time(NULL);
    g_ctx.total_encryptions = 0;
    g_ctx.total_decryptions = 0;
    g_ctx.initialized = 1;
    g_ctx.ref_count = 1;
    g_library_initialized = 1;
    g_library_start_time = time(NULL);
    g_session_mgr.count = 0;
    g_session_mgr.total_sessions_created = 0;
    g_session_mgr.total_sessions_destroyed = 0;

    bank_rng_init();

    BANK_LOG(BANK_LOG_INFO, "Cryptographic module initialized with random master key");
    return 1;
}

int bank_init_master(const uint8_t *key, size_t key_len, const uint8_t *salt, size_t salt_len)
{
    if (key_len != BANK_AES_KEY_BYTES || salt_len != 32)
        return 0;

    memcpy(g_ctx.master_key, key, BANK_AES_KEY_BYTES);
    memcpy(g_ctx.salt, salt, 32);
    bank_random_bytes(g_ctx.anti_tamper_seed, 32);
    bank_random_bytes(g_ctx.key_rotation_seed, 32);

#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&g_ctx.mutex);
    InitializeCriticalSection(&g_ctx.rng_mutex);
    InitializeCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_init(&g_ctx.mutex, NULL);
    pthread_mutex_init(&g_ctx.rng_mutex, NULL);
    pthread_mutex_init(&g_session_mgr.mutex, NULL);
#endif

    memcpy(g_ctx.key_chain[0], g_ctx.master_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = 0;
    g_ctx.key_created_at = time(NULL);
    g_ctx.total_encryptions = 0;
    g_ctx.total_decryptions = 0;
    g_ctx.initialized = 1;
    g_ctx.ref_count = 1;
    g_library_initialized = 1;
    g_library_start_time = time(NULL);
    g_session_mgr.count = 0;

    bank_rng_init();

    BANK_LOG(BANK_LOG_INFO, "Cryptographic module initialized with provided master key");
    return 1;
}

void bank_rotate_master_key(void)
{
    if (!g_ctx.initialized)
        return;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_ctx.mutex);
#else
    pthread_mutex_lock(&g_ctx.mutex);
#endif

    int new_index = (g_ctx.current_key_index + 1) % BANK_MAX_KEY_ROTATIONS;
    uint8_t new_key[BANK_AES_KEY_BYTES];
    uint8_t hash_input[64];

    memcpy(hash_input, g_ctx.key_chain[g_ctx.current_key_index], BANK_AES_KEY_BYTES);
    memcpy(hash_input + BANK_AES_KEY_BYTES, g_ctx.key_rotation_seed, 32);
    bank_sha256(hash_input, BANK_AES_KEY_BYTES + 32, new_key);
    memcpy(g_ctx.key_chain[new_index], new_key, BANK_AES_KEY_BYTES);
    g_ctx.current_key_index = new_index;
    g_ctx.key_created_at = time(NULL);

    bank_secure_zero(new_key, sizeof(new_key));
    bank_secure_zero(hash_input, sizeof(hash_input));

    BANK_LOG(BANK_LOG_INFO, "Master key rotated to index %d", new_index);

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_ctx.mutex);
#else
    pthread_mutex_unlock(&g_ctx.mutex);
#endif
}

void bank_wipe_master(void)
{
    if (!g_ctx.initialized)
        return;

    bank_secure_zero(&g_ctx, sizeof(g_ctx));

#if defined(_WIN32) || defined(_WIN64)
    DeleteCriticalSection(&g_ctx.mutex);
    DeleteCriticalSection(&g_ctx.rng_mutex);
    DeleteCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_destroy(&g_ctx.mutex);
    pthread_mutex_destroy(&g_ctx.rng_mutex);
    pthread_mutex_destroy(&g_session_mgr.mutex);
#endif

    g_ctx.initialized = 0;
    g_library_initialized = 0;

    BANK_LOG(BANK_LOG_INFO, "Cryptographic module wiped");
}

static int bank_derive_session_key(const uint8_t *key_id, size_t key_id_len, uint8_t *session_key, uint8_t *hmac_key)
{
    uint8_t data[64];
    uint8_t derived[BANK_AES_KEY_BYTES * 2];

    if (key_id_len > 32)
        key_id_len = 32;

    memcpy(data, g_ctx.salt, 32);
    memcpy(data + 32, key_id, key_id_len);

    bank_pbkdf2_hmac_sha256(g_ctx.key_chain[g_ctx.current_key_index], BANK_AES_KEY_BYTES,
                            data, 32 + key_id_len, BANK_PBKDF2_ROUNDS, derived, BANK_AES_KEY_BYTES * 2);

    memcpy(session_key, derived, BANK_AES_KEY_BYTES);
    memcpy(hmac_key, derived + BANK_AES_KEY_BYTES, BANK_AES_KEY_BYTES);
    bank_secure_zero(derived, sizeof(derived));

    return 1;
}

int bank_create_session(const uint8_t *key_id, size_t key_id_len, int ttl_seconds)
{
    if (!g_ctx.initialized)
        return 0;
    if (key_id_len > BANK_MAX_KEY_ID)
        return 0;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_lock(&g_session_mgr.mutex);
#endif

    if (g_session_mgr.count >= BANK_MAX_SESSION_KEYS)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mgr.mutex);
#else
        pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
        BANK_LOG(BANK_LOG_WARN, "Session creation failed: max sessions reached");
        return 0;
    }

    int slot = -1;
    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (!g_session_mgr.keys[i].active)
        {
            slot = i;
            break;
        }
    }

    if (slot == -1)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mgr.mutex);
#else
        pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
        return 0;
    }

    if (!bank_derive_session_key(key_id, key_id_len, g_session_mgr.keys[slot].session_key,
                                 g_session_mgr.keys[slot].hmac_key))
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mgr.mutex);
#else
        pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
        return 0;
    }

    memcpy(g_session_mgr.keys[slot].key_id, key_id, key_id_len);
    g_session_mgr.keys[slot].key_id_len = key_id_len;
    g_session_mgr.keys[slot].created_at = time(NULL);
    g_session_mgr.keys[slot].expires_at = time(NULL) + ttl_seconds;
    g_session_mgr.keys[slot].last_used_at = time(NULL);
    g_session_mgr.keys[slot].use_count = 0;
    g_session_mgr.keys[slot].active = 1;
    bank_random_bytes(g_session_mgr.keys[slot].salt, 16);
    g_session_mgr.count++;
    g_session_mgr.total_sessions_created++;

    BANK_LOG(BANK_LOG_INFO, "Session created with ID %d, TTL %d seconds", slot + 1, ttl_seconds);

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_unlock(&g_session_mgr.mutex);
#endif

    return slot + 1;
}

int bank_get_session_key(int session_id, uint8_t *session_key, uint8_t *hmac_key)
{
    if (!g_ctx.initialized)
        return 0;
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return 0;

    int idx = session_id - 1;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_lock(&g_session_mgr.mutex);
#endif

    if (!g_session_mgr.keys[idx].active)
    {
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mgr.mutex);
#else
        pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
        return 0;
    }

    if (time(NULL) > g_session_mgr.keys[idx].expires_at)
    {
        g_session_mgr.keys[idx].active = 0;
        bank_secure_zero(g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);
        g_session_mgr.count--;
        g_session_mgr.total_sessions_destroyed++;
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_session_mgr.mutex);
#else
        pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
        BANK_LOG(BANK_LOG_WARN, "Session %d expired", session_id);
        return 0;
    }

    memcpy(session_key, g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
    if (hmac_key)
        memcpy(hmac_key, g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);

    g_session_mgr.keys[idx].last_used_at = time(NULL);
    g_session_mgr.keys[idx].use_count++;

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_unlock(&g_session_mgr.mutex);
#endif

    return 1;
}

void bank_destroy_session(int session_id)
{
    if (session_id < 1 || session_id > BANK_MAX_SESSION_KEYS)
        return;

    int idx = session_id - 1;

#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_lock(&g_session_mgr.mutex);
#endif

    if (g_session_mgr.keys[idx].active)
    {
        bank_secure_zero(g_session_mgr.keys[idx].session_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].hmac_key, BANK_AES_KEY_BYTES);
        bank_secure_zero(g_session_mgr.keys[idx].key_id, BANK_MAX_KEY_ID);
        g_session_mgr.keys[idx].active = 0;
        g_session_mgr.count--;
        g_session_mgr.total_sessions_destroyed++;
        BANK_LOG(BANK_LOG_INFO, "Session %d destroyed", session_id);
    }

#if defined(_WIN32) || defined(_WIN64)
    LeaveCriticalSection(&g_session_mgr.mutex);
#else
    pthread_mutex_unlock(&g_session_mgr.mutex);
#endif
}

static int bank_secure_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    volatile uint8_t result = 0;

#if defined(_WIN32) || defined(_WIN64)
    Sleep(BANK_SECURE_COMPARE_DELAY_MS);
#else
    usleep(BANK_SECURE_COMPARE_DELAY_MS * 1000);
#endif

    for (size_t i = 0; i < len; i++)
    {
        result |= a[i] ^ b[i];
    }

#if defined(_WIN32) || defined(_WIN64)
    Sleep(BANK_SECURE_COMPARE_DELAY_MS);
#else
    usleep(BANK_SECURE_COMPARE_DELAY_MS * 1000);
#endif

    return result == 0;
}

int bank_encrypt_data(const uint8_t *plain, size_t plain_len,
                      const uint8_t *aad, size_t aad_len,
                      int session_id,
                      uint8_t *cipher, size_t *cipher_len,
                      uint8_t iv[BANK_GCM_IV_BYTES],
                      uint8_t tag[BANK_GCM_TAG_BYTES])
{
    if (!g_ctx.initialized)
        return 0;
    if (!plain || !cipher || !cipher_len || !iv || !tag)
        return 0;
    if (plain_len > BANK_MAX_DATA)
        return 0;
    if (!bank_rng_bytes(iv, BANK_GCM_IV_BYTES))
        return 0;

    uint8_t session_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, NULL))
        return 0;

    int result = bank_aes256_gcm_encrypt(session_key, iv, BANK_GCM_IV_BYTES,
                                         aad, aad_len, plain, plain_len,
                                         cipher, tag);

    if (result)
    {
        *cipher_len = plain_len;
#if defined(_WIN32) || defined(_WIN64)
        EnterCriticalSection(&g_ctx.mutex);
#else
        pthread_mutex_lock(&g_ctx.mutex);
#endif
        g_ctx.total_encryptions++;
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_ctx.mutex);
#else
        pthread_mutex_unlock(&g_ctx.mutex);
#endif
    }

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);

    return result;
}

int bank_decrypt_data(const uint8_t *cipher, size_t cipher_len,
                      const uint8_t *aad, size_t aad_len,
                      int session_id,
                      const uint8_t iv[BANK_GCM_IV_BYTES],
                      const uint8_t tag[BANK_GCM_TAG_BYTES],
                      uint8_t *plain, size_t *plain_len)
{
    if (!g_ctx.initialized)
        return 0;
    if (!cipher || !iv || !tag || !plain || !plain_len)
        return 0;
    if (cipher_len > BANK_MAX_DATA)
        return 0;

    uint8_t session_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, NULL))
        return 0;

    int result = bank_aes256_gcm_decrypt(session_key, iv, BANK_GCM_IV_BYTES,
                                         aad, aad_len, cipher, cipher_len,
                                         tag, plain);

    if (result)
    {
        *plain_len = cipher_len;
#if defined(_WIN32) || defined(_WIN64)
        EnterCriticalSection(&g_ctx.mutex);
#else
        pthread_mutex_lock(&g_ctx.mutex);
#endif
        g_ctx.total_decryptions++;
#if defined(_WIN32) || defined(_WIN64)
        LeaveCriticalSection(&g_ctx.mutex);
#else
        pthread_mutex_unlock(&g_ctx.mutex);
#endif
    }

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);

    return result;
}

int bank_sign_data(int session_id, const uint8_t *data, size_t data_len, uint8_t signature[BANK_SHA256_DIGEST_SIZE])
{
    if (!g_ctx.initialized)
        return 0;
    if (!data || !signature)
        return 0;

    uint8_t hmac_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, NULL, hmac_key))
        return 0;

    bank_hmac_sha256(hmac_key, BANK_AES_KEY_BYTES, data, data_len, signature);

    bank_secure_zero(hmac_key, BANK_AES_KEY_BYTES);

    return 1;
}

int bank_verify_signature(int session_id, const uint8_t *data, size_t data_len, const uint8_t signature[BANK_SHA256_DIGEST_SIZE])
{
    uint8_t expected[BANK_SHA256_DIGEST_SIZE];
    if (!bank_sign_data(session_id, data, data_len, expected))
        return 0;

    return bank_secure_compare(signature, expected, BANK_SHA256_DIGEST_SIZE);
}

uint32_t bank_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t poly = 0xEDB88320;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
        }
    }

    return ~crc;
}

uint32_t bank_crc32_combine(uint32_t crc1, uint32_t crc2, size_t len2)
{
    uint32_t crc = crc1;
    uint32_t power = 1;

    for (size_t i = 0; i < len2; i++)
    {
        power = (power << 1) ^ ((power & 0x80000000) ? 0xEDB88320 : 0);
    }

    for (size_t i = 0; i < 32; i++)
    {
        if (power & (1 << i))
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }

    return crc ^ crc2;
}

int bank_create_envelope(const uint8_t *plain, size_t plain_len,
                         const uint8_t *aad, size_t aad_len,
                         int session_id,
                         bank_secure_envelope *envelope)
{
    if (!plain || !envelope)
        return 0;
    if (plain_len > BANK_MAX_DATA - 1024)
        return 0;

    memset(envelope, 0, sizeof(bank_secure_envelope));

    envelope->version = 2;
    envelope->compression_flag = 0;

    if (!bank_encrypt_data(plain, plain_len, aad, aad_len, session_id,
                           envelope->encrypted_data, &envelope->encrypted_len,
                           envelope->iv, envelope->tag))
    {
        return 0;
    }

    uint8_t session_key[BANK_AES_KEY_BYTES];
    uint8_t hmac_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, hmac_key))
        return 0;

    uint8_t hash_input[64];
    memcpy(hash_input, session_key, BANK_AES_KEY_BYTES);
    memcpy(hash_input + BANK_AES_KEY_BYTES, hmac_key, BANK_AES_KEY_BYTES);
    bank_sha256(hash_input, BANK_AES_KEY_BYTES * 2, envelope->auth_tag);

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);
    bank_secure_zero(hmac_key, BANK_AES_KEY_BYTES);

    envelope->timestamp = time(NULL);
    envelope->crc32 = bank_crc32(envelope->encrypted_data, envelope->encrypted_len);

    BANK_LOG(BANK_LOG_DEBUG, "Created secure envelope, size: %zu bytes", envelope->encrypted_len);

    return 1;
}

int bank_extract_envelope(const bank_secure_envelope *envelope,
                          const uint8_t *aad, size_t aad_len,
                          uint8_t *plain, size_t *plain_len)
{
    if (!envelope || !plain || !plain_len)
        return 0;

    uint32_t crc = bank_crc32(envelope->encrypted_data, envelope->encrypted_len);
    if (crc != envelope->crc32)
    {
        BANK_LOG(BANK_LOG_ERROR, "Envelope CRC32 verification failed");
        return 0;
    }

    int session_id = -1;
    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (g_session_mgr.keys[i].active)
        {
            uint8_t expected_tag[32];
            uint8_t hash_input[64];
            memcpy(hash_input, g_session_mgr.keys[i].session_key, BANK_AES_KEY_BYTES);
            memcpy(hash_input + BANK_AES_KEY_BYTES, g_session_mgr.keys[i].hmac_key, BANK_AES_KEY_BYTES);
            bank_sha256(hash_input, BANK_AES_KEY_BYTES * 2, expected_tag);

            if (bank_secure_compare(envelope->auth_tag, expected_tag, 32))
            {
                session_id = i + 1;
                break;
            }
        }
    }

    if (session_id == -1)
    {
        BANK_LOG(BANK_LOG_ERROR, "Envelope authentication failed: no matching session key");
        return 0;
    }

    int result = bank_decrypt_data(envelope->encrypted_data, envelope->encrypted_len,
                                   aad, aad_len, session_id,
                                   envelope->iv, envelope->tag,
                                   plain, plain_len);

    if (result)
    {
        BANK_LOG(BANK_LOG_DEBUG, "Extracted envelope successfully");
    }

    return result;
}

int bank_create_transaction(uint64_t tx_id, const uint8_t *from, const uint8_t *to,
                            uint64_t amount, const uint8_t *currency,
                            int session_id, bank_transaction *tx)
{
    if (!from || !to || !currency || !tx)
        return 0;

    memset(tx, 0, sizeof(bank_transaction));

    tx->transaction_id = tx_id;
    tx->timestamp = time(NULL);
    memcpy(tx->from_account, from, 32);
    memcpy(tx->to_account, to, 32);
    tx->amount = amount;
    memcpy(tx->currency, currency, 8);

    if (!bank_rng_bytes((uint8_t *)&tx->nonce, 8))
        return 0;

    uint8_t data[512];
    size_t data_len = 0;
    memcpy(data + data_len, &tx->transaction_id, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->timestamp, 8);
    data_len += 8;
    memcpy(data + data_len, tx->from_account, 32);
    data_len += 32;
    memcpy(data + data_len, tx->to_account, 32);
    data_len += 32;
    memcpy(data + data_len, &tx->amount, 8);
    data_len += 8;
    memcpy(data + data_len, tx->currency, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->nonce, 8);
    data_len += 8;

    if (!bank_sign_data(session_id, data, data_len, tx->signature))
        return 0;

    bank_sha256(data, data_len, tx->reference_hash);

    tx->verified = 1;

    BANK_LOG(BANK_LOG_INFO, "Created transaction %llu for amount %llu",
             (unsigned long long)tx_id, (unsigned long long)amount);

    return 1;
}

int bank_verify_transaction(const bank_transaction *tx, int session_id)
{
    if (!tx || !tx->verified)
        return 0;

    uint8_t data[512];
    size_t data_len = 0;
    memcpy(data + data_len, &tx->transaction_id, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->timestamp, 8);
    data_len += 8;
    memcpy(data + data_len, tx->from_account, 32);
    data_len += 32;
    memcpy(data + data_len, tx->to_account, 32);
    data_len += 32;
    memcpy(data + data_len, &tx->amount, 8);
    data_len += 8;
    memcpy(data + data_len, tx->currency, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->nonce, 8);
    data_len += 8;

    uint8_t expected_hash[32];
    bank_sha256(data, data_len, expected_hash);

    if (!bank_secure_compare(tx->reference_hash, expected_hash, 32))
        return 0;

    return bank_verify_signature(session_id, data, data_len, tx->signature);
}

int bank_encrypt_chacha20_poly1305(const uint8_t *key, const uint8_t *nonce,
                                   const uint8_t *aad, size_t aad_len,
                                   const uint8_t *plain, size_t plain_len,
                                   uint8_t *cipher, uint8_t *tag)
{
    bank_chacha20_ctx chacha_ctx;
    bank_poly1305_ctx poly_ctx;
    uint8_t poly_key[BANK_POLY1305_KEY_SIZE];

    memcpy(chacha_ctx.key, key, BANK_AES_KEY_BYTES);
    memcpy(chacha_ctx.nonce, nonce, 12);
    chacha_ctx.counter = 0;

    bank_chacha20_encrypt(&chacha_ctx, plain, plain_len, cipher);

    chacha_ctx.counter = 0;
    uint8_t zero_block[64] = {0};
    uint8_t poly_key_block[64];
    bank_chacha20_encrypt(&chacha_ctx, zero_block, 64, poly_key_block);
    memcpy(poly_key, poly_key_block, BANK_POLY1305_KEY_SIZE);

    bank_poly1305_init(&poly_ctx, poly_key);
    bank_poly1305_update(&poly_ctx, aad, aad_len);

    size_t padding = (16 - (aad_len % 16)) % 16;
    uint8_t padding_buf[16] = {0};
    bank_poly1305_update(&poly_ctx, padding_buf, padding);

    bank_poly1305_update(&poly_ctx, cipher, plain_len);

    padding = (16 - (plain_len % 16)) % 16;
    bank_poly1305_update(&poly_ctx, padding_buf, padding);

    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t cipher_bits = plain_len * 8;
    memcpy(len_block, &aad_bits, 8);
    memcpy(len_block + 8, &cipher_bits, 8);
    bank_poly1305_update(&poly_ctx, len_block, 16);

    bank_poly1305_final(&poly_ctx, tag);

    bank_secure_zero(&chacha_ctx, sizeof(chacha_ctx));
    bank_secure_zero(&poly_ctx, sizeof(poly_ctx));
    bank_secure_zero(poly_key, sizeof(poly_key));

    return 1;
}

int bank_derive_key_from_password(const uint8_t *password, size_t password_len,
                                  const uint8_t *salt, size_t salt_len,
                                  uint8_t *key, size_t key_len)
{
    if (!password || !salt || !key)
        return 0;

    bank_pbkdf2_hmac_sha256(password, password_len, salt, salt_len,
                            BANK_PBKDF2_ROUNDS, key, key_len);

    return 1;
}

void bank_self_test(void)
{
    uint8_t key[32];
    uint8_t iv[12];
    uint8_t plain[128];
    uint8_t cipher[128];
    uint8_t dec[128];
    uint8_t tag[16];
    uint8_t hash[32];
    uint8_t signature[32];
    uint8_t poly_tag[BANK_POLY1305_TAG_SIZE];
    bank_transaction tx;
    bank_secure_envelope env;
    int session_id;
    size_t out_len;

    for (int i = 0; i < 32; i++)
        key[i] = i & 0xFF;
    for (int i = 0; i < 12; i++)
        iv[i] = i & 0xFF;
    for (int i = 0; i < 128; i++)
        plain[i] = i & 0xFF;

    if (!bank_aes256_gcm_encrypt(key, iv, 12, NULL, 0, plain, 128, cipher, tag))
    {
        BANK_LOG(BANK_LOG_FATAL, "AES-GCM encrypt self-test failed");
        exit(1);
    }

    if (!bank_aes256_gcm_decrypt(key, iv, 12, NULL, 0, cipher, 128, tag, dec))
    {
        BANK_LOG(BANK_LOG_FATAL, "AES-GCM decrypt self-test failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "AES-GCM integrity self-test failed");
        exit(1);
    }

    bank_sha256(plain, 128, hash);

    bank_hmac_sha256(key, 32, plain, 128, signature);

    uint8_t expected_sig[32];
    bank_hmac_sha256(key, 32, plain, 128, expected_sig);
    if (memcmp(signature, expected_sig, 32) != 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "HMAC-SHA256 self-test failed");
        exit(1);
    }

    uint8_t chacha_key[32];
    uint8_t chacha_nonce[12];
    uint8_t chacha_plain[64];
    uint8_t chacha_cipher[64];
    uint8_t chacha_dec[64];

    for (int i = 0; i < 32; i++)
        chacha_key[i] = i;
    for (int i = 0; i < 12; i++)
        chacha_nonce[i] = i;
    for (int i = 0; i < 64; i++)
        chacha_plain[i] = i;

    bank_chacha20_ctx chacha_ctx;
    memcpy(chacha_ctx.key, chacha_key, 32);
    memcpy(chacha_ctx.nonce, chacha_nonce, 12);
    chacha_ctx.counter = 0;
    bank_chacha20_encrypt(&chacha_ctx, chacha_plain, 64, chacha_cipher);

    memcpy(chacha_ctx.key, chacha_key, 32);
    memcpy(chacha_ctx.nonce, chacha_nonce, 12);
    chacha_ctx.counter = 0;
    bank_chacha20_encrypt(&chacha_ctx, chacha_cipher, 64, chacha_dec);

    if (memcmp(chacha_plain, chacha_dec, 64) != 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "ChaCha20 self-test failed");
        exit(1);
    }

    if (!bank_encrypt_chacha20_poly1305(key, iv, NULL, 0, plain, 128, cipher, poly_tag))
    {
        BANK_LOG(BANK_LOG_FATAL, "ChaCha20-Poly1305 encrypt self-test failed");
        exit(1);
    }

    if (!bank_init_master_random())
    {
        BANK_LOG(BANK_LOG_FATAL, "Master key init failed");
        exit(1);
    }

    session_id = bank_create_session((const uint8_t *)"test_key_self_test", 19, 3600);
    if (session_id <= 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "Session creation failed");
        exit(1);
    }

    if (!bank_encrypt_data(plain, 128, NULL, 0, session_id, cipher, &out_len, iv, tag))
    {
        BANK_LOG(BANK_LOG_FATAL, "Bank encrypt failed");
        exit(1);
    }

    if (!bank_decrypt_data(cipher, out_len, NULL, 0, session_id, iv, tag, dec, &out_len))
    {
        BANK_LOG(BANK_LOG_FATAL, "Bank decrypt failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "Bank encrypt/decrypt integrity failed");
        exit(1);
    }

    if (!bank_sign_data(session_id, plain, 128, signature))
    {
        BANK_LOG(BANK_LOG_FATAL, "Signing failed");
        exit(1);
    }

    if (!bank_verify_signature(session_id, plain, 128, signature))
    {
        BANK_LOG(BANK_LOG_FATAL, "Signature verification failed");
        exit(1);
    }

    if (!bank_create_transaction(12345678, plain, plain + 32, 1000000, (const uint8_t *)"USD", session_id, &tx))
    {
        BANK_LOG(BANK_LOG_FATAL, "Transaction creation failed");
        exit(1);
    }

    if (!bank_verify_transaction(&tx, session_id))
    {
        BANK_LOG(BANK_LOG_FATAL, "Transaction verification failed");
        exit(1);
    }

    if (!bank_create_envelope(plain, 128, NULL, 0, session_id, &env))
    {
        BANK_LOG(BANK_LOG_FATAL, "Envelope creation failed");
        exit(1);
    }

    if (!bank_extract_envelope(&env, NULL, 0, dec, &out_len))
    {
        BANK_LOG(BANK_LOG_FATAL, "Envelope extraction failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "Envelope integrity failed");
        exit(1);
    }

    uint8_t derived_key[32];
    uint8_t password[] = "test_password";
    uint8_t salt[] = "random_salt_12345";
    if (!bank_derive_key_from_password(password, sizeof(password) - 1, salt, sizeof(salt) - 1, derived_key, 32))
    {
        BANK_LOG(BANK_LOG_FATAL, "Key derivation failed");
        exit(1);
    }

    bank_rotate_master_key();

    int session_id2 = bank_create_session((const uint8_t *)"test_key_after_rotation", 24, 3600);
    if (session_id2 <= 0)
    {
        BANK_LOG(BANK_LOG_FATAL, "Session creation after rotation failed");
        exit(1);
    }

    bank_destroy_session(session_id);
    bank_destroy_session(session_id2);
    bank_wipe_master();

    BANK_LOG(BANK_LOG_INFO, "All self-tests passed successfully");
    fprintf(stderr, "All self-tests passed successfully\n");
}

void bank_get_stats(uint64_t *total_encryptions, uint64_t *total_decryptions,
                    uint64_t *total_sessions_created, uint64_t *total_sessions_destroyed,
                    int *active_sessions)
{
    if (total_encryptions)
        *total_encryptions = g_ctx.total_encryptions;
    if (total_decryptions)
        *total_decryptions = g_ctx.total_decryptions;
    if (total_sessions_created)
        *total_sessions_created = g_session_mgr.total_sessions_created;
    if (total_sessions_destroyed)
        *total_sessions_destroyed = g_session_mgr.total_sessions_destroyed;
    if (active_sessions)
        *active_sessions = g_session_mgr.count;
}

int bank_is_initialized(void)
{
    return g_library_initialized;
}

time_t bank_get_library_uptime(void)
{
    if (!g_library_initialized)
        return 0;
    return time(NULL) - g_library_start_time;
}

int main(void)
{
    bank_set_log_level(BANK_LOG_INFO);
    bank_set_log_file("bank_crypto.log");

    bank_self_test();

    printf("\n");
    printf("========================================================================\n");
    printf("                   BANK CRYPTOGRAPHIC MODULE v3.0                       \n");
    printf("========================================================================\n");
    printf("Status: OPERATIONAL\n");
    printf("Algorithms: AES-256-GCM, ChaCha20-Poly1305, SHA-256, SHA-512, HMAC-SHA256\n");
    printf("           PBKDF2-HMAC-SHA256, HKDF, Poly1305, CRC32\n");
    printf("------------------------------------------------------------------------\n");
    printf("Configuration:\n");
    printf("  Max data size: %d bytes (%.2f MB)\n", BANK_MAX_DATA, (double)BANK_MAX_DATA / (1024 * 1024));
    printf("  Max sessions: %d\n", BANK_MAX_SESSION_KEYS);
    printf("  PBKDF2 iterations: %d\n", BANK_PBKDF2_ROUNDS);
    printf("  RNG pool size: %d bytes\n", BANK_RNG_POOL_SIZE);
    printf("  Max key rotations: %d\n", BANK_MAX_KEY_ROTATIONS);
    printf("------------------------------------------------------------------------\n");
    printf("Log file: bank_crypto.log\n");
    printf("========================================================================\n\n");

    printf("API Functions:\n");
    printf("  [INITIALIZATION]\n");
    printf("    bank_init_master_random() - Initialize with random master key\n");
    printf("    bank_init_master() - Initialize with provided master key\n");
    printf("    bank_rotate_master_key() - Rotate master key securely\n");
    printf("    bank_wipe_master() - Securely wipe all cryptographic material\n");
    printf("\n  [SESSION MANAGEMENT]\n");
    printf("    bank_create_session() - Create encryption session with TTL\n");
    printf("    bank_destroy_session() - Destroy session and wipe keys\n");
    printf("    bank_get_session_key() - Retrieve session keys\n");
    printf("\n  [ENCRYPTION/DECRYPTION]\n");
    printf("    bank_encrypt_data() - AES-256-GCM encryption\n");
    printf("    bank_decrypt_data() - AES-256-GCM decryption\n");
    printf("    bank_encrypt_chacha20_poly1305() - ChaCha20-Poly1305 encryption\n");
    printf("\n  [AUTHENTICATION]\n");
    printf("    bank_sign_data() / bank_verify_signature() - HMAC-SHA256\n");
    printf("    bank_hmac_sha256() - Direct HMAC computation\n");
    printf("\n  [TRANSACTIONS]\n");
    printf("    bank_create_transaction() - Create signed financial transaction\n");
    printf("    bank_verify_transaction() - Verify transaction signature\n");
    printf("\n  [SECURE ENVELOPES]\n");
    printf("    bank_create_envelope() - Create authenticated secure envelope\n");
    printf("    bank_extract_envelope() - Extract and verify envelope\n");
    printf("\n  [HASH & KEY DERIVATION]\n");
    printf("    bank_sha256() / bank_sha512() - Cryptographic hashing\n");
    printf("    bank_pbkdf2_hmac_sha256() - Password-based key derivation\n");
    printf("    bank_hkdf_extract() / bank_hkdf_expand() - HKDF key derivation\n");
    printf("    bank_derive_key_from_password() - High-level key derivation\n");
    printf("\n  [UTILITIES]\n");
    printf("    bank_random_bytes() - Cryptographically secure random\n");
    printf("    bank_crc32() / bank_crc32_combine() - CRC32 checksum\n");
    printf("    bank_secure_zero() - Secure memory zeroing\n");
    printf("    bank_secure_compare() - Timing-safe comparison\n");
    printf("    bank_get_stats() - Get library statistics\n");
    printf("========================================================================\n");

    uint64_t enc, dec, sess_created, sess_destroyed;
    int active;
    bank_get_stats(&enc, &dec, &sess_created, &sess_destroyed, &active);

    printf("\nRuntime Statistics:\n");
    printf("  Total encryptions: %llu\n", (unsigned long long)enc);
    printf("  Total decryptions: %llu\n", (unsigned long long)dec);
    printf("  Sessions created: %llu\n", (unsigned long long)sess_created);
    printf("  Sessions destroyed: %llu\n", (unsigned long long)sess_destroyed);
    printf("  Active sessions: %d\n", active);
    printf("  Library uptime: %ld seconds\n", (long)bank_get_library_uptime());
    printf("========================================================================\n\n");

    printf("Security Notes:\n");
    printf("  - All keys are zeroed after use\n");
    printf("  - Timing-safe comparison implemented\n");
    printf("  - Master key supports rotation\n");
    printf("  - Session keys have TTL expiration\n");
    printf("  - RNG with entropy pool\n");
    printf("  - Anti-tamper mechanisms active\n");
    printf("========================================================================\n");

    bank_close_log_file();

    return 0;
}
