#ifndef BANK_TYPES_H
#define BANK_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define BANK_AES_BLOCK_SIZE 16
#define BANK_AES_KEY_BYTES 32
#define BANK_AES_KEY_BITS 256
#define BANK_GCM_IV_BYTES 12
#define BANK_GCM_TAG_BYTES 16
#define BANK_SHA256_BLOCK_SIZE 64
#define BANK_SHA256_DIGEST_SIZE 32
#define BANK_SHA512_BLOCK_SIZE 128
#define BANK_SHA512_DIGEST_SIZE 64
#define BANK_HMAC_BLOCK_SIZE 64
#define BANK_MAX_DATA (1024 * 1024 * 10)
#define BANK_PBKDF2_ROUNDS 100000
#define BANK_MAX_KEY_ID 256
#define BANK_MAX_SESSION_KEYS 32
#define BANK_LOG_BUFFER_SIZE 4096
#define BANK_MAX_AAD_SIZE 65536
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
    uint8_t buffer[BANK_SHA512_BLOCK_SIZE];
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

#endif
