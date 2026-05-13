#ifndef P2P_CORE_CRYPTO_H
#define P2P_CORE_CRYPTO_H

#include "../include/common.h"

int crypto_init(void);
void crypto_cleanup(void);
int random_bytes(uint8_t *buf, size_t len);
void sha256(const uint8_t *data, size_t len, uint8_t *hash);
uint32_t crc32(const uint8_t *data, size_t len);
void simple_xor_encrypt(uint8_t *data, size_t len, const uint8_t *key, size_t key_len);
void simple_xor_decrypt(uint8_t *data, size_t len, const uint8_t *key, size_t key_len);
int secure_compare(const uint8_t *a, const uint8_t *b, size_t len);
void secure_zero(void *ptr, size_t len);

#endif
