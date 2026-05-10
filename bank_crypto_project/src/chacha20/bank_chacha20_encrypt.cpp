#include "bank/bank_chacha20.h"
#include "bank/bank_utils.h"

int bank_encrypt_chacha20_poly1305(const uint8_t *key, const uint8_t *nonce,
                                   const uint8_t *aad, size_t aad_len,
                                   const uint8_t *plain, size_t plain_len,
                                   uint8_t *cipher, uint8_t *tag)
{
    bank_chacha20_ctx chacha_ctx;
    bank_poly1305_ctx poly_ctx;
    uint8_t poly_key[BANK_POLY1305_KEY_SIZE];

    memcpy(chacha_ctx.key, key, BANK_AES_KEY_BYTES);
    memcpy(chacha_ctx.nonce, nonce, 12);
    chacha_ctx.counter = 0;

    bank_chacha20_encrypt(&chacha_ctx, plain, plain_len, cipher);

    chacha_ctx.counter = 0;
    uint8_t zero_block[64] = {0};
    uint8_t poly_key_block[64];
    bank_chacha20_encrypt(&chacha_ctx, zero_block, 64, poly_key_block);
    memcpy(poly_key, poly_key_block, BANK_POLY1305_KEY_SIZE);

    bank_poly1305_init(&poly_ctx, poly_key);
    bank_poly1305_update(&poly_ctx, aad, aad_len);

    size_t padding = (16 - (aad_len % 16)) % 16;
    uint8_t padding_buf[16] = {0};
    bank_poly1305_update(&poly_ctx, padding_buf, padding);

    bank_poly1305_update(&poly_ctx, cipher, plain_len);

    padding = (16 - (plain_len % 16)) % 16;
    bank_poly1305_update(&poly_ctx, padding_buf, padding);

    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t cipher_bits = plain_len * 8;
    memcpy(len_block, &aad_bits, 8);
    memcpy(len_block + 8, &cipher_bits, 8);
    bank_poly1305_update(&poly_ctx, len_block, 16);

    bank_poly1305_final(&poly_ctx, tag);

    bank_secure_zero(&chacha_ctx, sizeof(chacha_ctx));
    bank_secure_zero(&poly_ctx, sizeof(poly_ctx));
    bank_secure_zero(poly_key, sizeof(poly_key));

    return 1;
}
