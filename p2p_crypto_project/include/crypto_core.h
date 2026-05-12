#ifndef CRYPTO_CORE_H
#define CRYPTO_CORE_H

#include "crypto_types.h"

void init_sboxes(CryptoContext *ctx);
void init_gf256_tables(CryptoContext *ctx);
void init_permutation(CryptoContext *ctx);
void init_round_constants(CryptoContext *ctx);
void init_entropy_pool(CryptoContext *ctx);
void collect_system_entropy(CryptoContext *ctx);
void hash_core(CryptoContext *ctx, const byte *data, size_t data_len);
void apply_signature_algorithm(CryptoContext *ctx, size_t len);
int verify_signature_core(CryptoContext *ctx, const byte *signature, size_t sig_len);
void apply_key_derivation(CryptoContext *ctx, byte *key, size_t key_len);
void compute_exchange_phase1(CryptoContext *ctx, const byte *input, size_t len);
void compute_exchange_phase2(CryptoContext *ctx, const byte *input, size_t len);
void setup_encryption_key(CryptoContext *ctx, const byte *key, size_t key_len);
void setup_decryption_key(CryptoContext *ctx, const byte *key, size_t key_len);
void wipe_all_secure_memory(CryptoContext *ctx);
u64 rdrand_wrapper(void);
u64 rdseed_wrapper(void);
u64 rdtsc_wrapper(void);

#endif
