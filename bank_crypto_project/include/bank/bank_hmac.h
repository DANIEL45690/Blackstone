#ifndef BANK_HMAC_H
#define BANK_HMAC_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac);
    void bank_hmac_sha256_init(bank_sha256_ctx *ctx, const uint8_t *key, size_t key_len);
    void bank_hmac_sha256_update(bank_sha256_ctx *ctx, const uint8_t *data, size_t data_len);
    void bank_hmac_sha256_final(bank_sha256_ctx *ctx, uint8_t *mac, const uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif
