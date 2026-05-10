#include "bank/bank_chacha20.h"
#include "bank/bank_utils.h"
#include <string.h>

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

void bank_chacha20_init(bank_chacha20_ctx *ctx, const uint8_t *key, const uint8_t *nonce, uint32_t counter)
{
    memcpy(ctx->key, key, BANK_AES_KEY_BYTES);
    memcpy(ctx->nonce, nonce, 12);
    ctx->counter = counter;
}

void bank_chacha20_block(bank_chacha20_ctx *ctx, uint8_t *output)
{
    uint32_t x[16];
    const uint8_t *key_ptr = ctx->key;
    const uint8_t *nonce_ptr = ctx->nonce;

    x[0] = 0x61707865;
    x[1] = 0x3320646e;
    x[2] = 0x79622d32;
    x[3] = 0x6b206574;

    for (int i = 0; i < 8; i++)
    {
        x[4 + i] = ((uint32_t)key_ptr[i * 4] << 24) |
                   ((uint32_t)key_ptr[i * 4 + 1] << 16) |
                   ((uint32_t)key_ptr[i * 4 + 2] << 8) |
                   ((uint32_t)key_ptr[i * 4 + 3]);
    }

    x[12] = ctx->counter;
    x[13] = ((uint32_t)nonce_ptr[0] << 24) |
            ((uint32_t)nonce_ptr[1] << 16) |
            ((uint32_t)nonce_ptr[2] << 8) |
            ((uint32_t)nonce_ptr[3]);
    x[14] = ((uint32_t)nonce_ptr[4] << 24) |
            ((uint32_t)nonce_ptr[5] << 16) |
            ((uint32_t)nonce_ptr[6] << 8) |
            ((uint32_t)nonce_ptr[7]);
    x[15] = ((uint32_t)nonce_ptr[8] << 24) |
            ((uint32_t)nonce_ptr[9] << 16) |
            ((uint32_t)nonce_ptr[10] << 8) |
            ((uint32_t)nonce_ptr[11]);

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

void bank_chacha20_set_counter(bank_chacha20_ctx *ctx, uint32_t counter)
{
    ctx->counter = counter;
}

uint32_t bank_chacha20_get_counter(const bank_chacha20_ctx *ctx)
{
    return ctx->counter;
}
