#include "bank/bank_gcm.h"
#include "bank/bank_utils.h"
#include <string.h>

static void bank_ghash_multiply(uint8_t *x, const uint8_t *h)
{
    uint8_t z = x[15];
    for (int k = 15; k >= 0; k--)
    {
        x[k] = (x[k] >> 1) ^ ((k > 0 ? (x[k - 1] & 1) : 0) << 7);
        if (z & 1)
            x[k] ^= h[k];
        z >>= 1;
    }
}

void bank_ghash(uint8_t *h, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t cipher_len, uint8_t *s)
{
    uint8_t x[16] = {0};
    uint8_t temp[16];

    size_t aad_blocks = (aad_len + 15) / 16;
    size_t cipher_blocks = (cipher_len + 15) / 16;

    for (size_t i = 0; i < aad_blocks; i++)
    {
        memset(temp, 0, 16);
        size_t block_len = (i == aad_blocks - 1 && aad_len % 16 != 0) ? aad_len % 16 : 16;
        memcpy(temp, aad + i * 16, block_len);

        for (size_t j = 0; j < 16; j++)
            x[j] ^= temp[j];

        bank_ghash_multiply(x, h);
    }

    for (size_t i = 0; i < cipher_blocks; i++)
    {
        memset(temp, 0, 16);
        size_t block_len = (i == cipher_blocks - 1 && cipher_len % 16 != 0) ? cipher_len % 16 : 16;
        memcpy(temp, ciphertext + i * 16, block_len);

        for (size_t j = 0; j < 16; j++)
            x[j] ^= temp[j];

        bank_ghash_multiply(x, h);
    }

    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t cipher_bits = cipher_len * 8;
    for (int i = 0; i < 8; i++)
    {
        len_block[i] = (aad_bits >> (56 - i * 8)) & 0xFF;
        len_block[8 + i] = (cipher_bits >> (56 - i * 8)) & 0xFF;
    }

    for (size_t i = 0; i < 16; i++)
        x[i] ^= len_block[i];

    bank_ghash_multiply(x, h);

    memcpy(s, x, 16);
}
