#ifndef BANK_POLY1305_H
#define BANK_POLY1305_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_poly1305_init(bank_poly1305_ctx *ctx, const uint8_t *key);
    void bank_poly1305_update(bank_poly1305_ctx *ctx, const uint8_t *data, size_t len);
    void bank_poly1305_final(bank_poly1305_ctx *ctx, uint8_t *tag);
    void bank_poly1305_add_block(bank_poly1305_ctx *ctx, const uint8_t *block, size_t len, int final_block);

#ifdef __cplusplus
}
#endif

#endif
