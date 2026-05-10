#ifndef BANK_CHACHA20_H
#define BANK_CHACHA20_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_chacha20_init(bank_chacha20_ctx *ctx, const uint8_t *key, const uint8_t *nonce, uint32_t counter);
    void bank_chacha20_block(bank_chacha20_ctx *ctx, uint8_t *output);
    void bank_chacha20_encrypt(bank_chacha20_ctx *ctx, const uint8_t *input, size_t len, uint8_t *output);
    void bank_chacha20_quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);
    void bank_chacha20_set_counter(bank_chacha20_ctx *ctx, uint32_t counter);
    uint32_t bank_chacha20_get_counter(const bank_chacha20_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif
