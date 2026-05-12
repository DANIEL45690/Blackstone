#ifndef CRYPTO_TYPES_H
#define CRYPTO_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t byte;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct
{
    u64 state[8];
    u64 round_keys[16];
    u64 ternary_state[8];
    byte binary_buffer[4096];
    byte ternary_buffer[4096];
    byte hash_buffer[64];
    byte signature_buffer[128];
    u64 key_schedule[64];
    byte sbox_byte[256];
    byte inv_sbox[256];
    byte ternary_sbox[256];
    byte ternary_inv_sbox[256];
    byte gf256_exp[256];
    byte gf256_log[256];
    u32 permutation_table[256];
    u32 inverse_perm[256];
    u64 round_constants[64];
    u64 crypto_counter;
    u64 security_flags;
    u64 entropy_pool[32];
    u64 random_buffer[64];
    u64 timing_mask;
    u64 cache_flush;
    u64 error_counter;
    volatile u8 lock_flag;
    u8 initialized;
} CryptoContext;

#endif
