#ifndef BANK_SHA_H
#define BANK_SHA_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_sha256_init(bank_sha256_ctx *ctx);
    void bank_sha256_update(bank_sha256_ctx *ctx, const uint8_t *data, size_t len);
    void bank_sha256_final(bank_sha256_ctx *ctx, uint8_t *hash);
    void bank_sha256(const uint8_t *data, size_t len, uint8_t *hash);

    void bank_sha512_init(bank_sha512_ctx *ctx);
    void bank_sha512_update(bank_sha512_ctx *ctx, const uint8_t *data, size_t len);
    void bank_sha512_final(bank_sha512_ctx *ctx, uint8_t *hash);
    void bank_sha512(const uint8_t *data, size_t len, uint8_t *hash);

    uint32_t bank_rotr32(uint32_t x, int n);
    uint64_t bank_rotr64(uint64_t x, int n);
    uint32_t bank_ch32(uint32_t x, uint32_t y, uint32_t z);
    uint32_t bank_maj32(uint32_t x, uint32_t y, uint32_t z);
    uint32_t bank_bsig0(uint32_t x);
    uint32_t bank_bsig1(uint32_t x);
    uint32_t bank_ssig0(uint32_t x);
    uint32_t bank_ssig1(uint32_t x);

    uint64_t bank_ch64(uint64_t x, uint64_t y, uint64_t z);
    uint64_t bank_maj64(uint64_t x, uint64_t y, uint64_t z);
    uint64_t bank_bsig0_64(uint64_t x);
    uint64_t bank_bsig1_64(uint64_t x);
    uint64_t bank_ssig0_64(uint64_t x);
    uint64_t bank_ssig1_64(uint64_t x);

    extern const uint32_t bank_sha256_k[64];
    extern const uint64_t bank_sha512_k[80];

#ifdef __cplusplus
}
#endif

#endif
