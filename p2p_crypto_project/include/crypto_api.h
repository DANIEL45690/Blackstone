#ifndef CRYPTO_API_H
#define CRYPTO_API_H

#include "crypto_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int crypto_init(CryptoContext *ctx);
    int crypto_shutdown(CryptoContext *ctx);
    size_t binary_encode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output);
    size_t binary_decode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output);
    size_t ternary_encode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output);
    size_t ternary_decode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output);
    int compute_hash_secure(CryptoContext *ctx, const byte *data, size_t data_len, byte *hash_out);
    int compute_signature(CryptoContext *ctx, const byte *data, size_t data_len, const byte *key, size_t key_len, byte *sig_out);
    int verify_signature(CryptoContext *ctx, const byte *data, size_t data_len, const byte *key, size_t key_len, const byte *signature);
    int generate_random_key(CryptoContext *ctx, byte *key_out, size_t key_len);
    int key_exchange(CryptoContext *ctx, const byte *public_key, size_t key_len, byte *shared_secret_out, byte *session_key_out);
    int encrypt_data(CryptoContext *ctx, const byte *plaintext, size_t text_len, const byte *key, size_t key_len, const byte *iv, byte *ciphertext_out);
    int decrypt_data(CryptoContext *ctx, const byte *ciphertext, size_t text_len, const byte *key, size_t key_len, const byte *iv, byte *plaintext_out);
    void secure_memset(volatile void *ptr, int value, size_t num);
    void secure_memcpy(void *dest, const void *src, size_t num);
    int secure_compare(const void *a, const void *b, size_t num);
    int random_bytes(CryptoContext *ctx, byte *buffer, size_t buffer_len);
    int entropy_add(CryptoContext *ctx, const byte *data, size_t data_len);
    int crypto_self_test(CryptoContext *ctx);
    void lock_critical_section(CryptoContext *ctx);
    void unlock_critical_section(CryptoContext *ctx);
    u64 get_security_status(CryptoContext *ctx);
    void wipe_memory(volatile void *ptr, size_t num);
    int protect_memory(void *ptr, size_t size);
    int unprotect_memory(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
