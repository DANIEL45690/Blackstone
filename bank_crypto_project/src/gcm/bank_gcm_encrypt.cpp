#include "bank/bank_gcm.h"
#include "bank/bank_aes.h"
#include "bank/bank_utils.h"
#include <string.h>

void bank_gctr(const bank_aes256_key *key, const uint8_t *icb, const uint8_t *x, size_t xlen, uint8_t *y)
{
    uint8_t cb[16];
    uint8_t enc_cb[16];
    size_t n_blocks = (xlen + 15) / 16;
    size_t last_block_len = xlen % 16;

    memcpy(cb, icb, 16);

    for (size_t i = 0; i < n_blocks; i++)
    {
        cb[15]++;
        if (cb[15] == 0)
        {
            for (int j = 14; j >= 0; j--)
            {
                cb[j]++;
                if (cb[j] != 0)
                    break;
            }
        }

        bank_aes_encrypt_block(cb, enc_cb, key);

        size_t block_len = (i == n_blocks - 1 && last_block_len > 0) ? last_block_len : 16;

        for (size_t j = 0; j < block_len; j++)
        {
            y[i * 16 + j] = x[i * 16 + j] ^ enc_cb[j];
        }
    }
}

int bank_aes256_gcm_encrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *plain, size_t plain_len,
                            uint8_t *cipher, uint8_t *tag)
{
    bank_aes256_key aes_key;
    uint8_t h[16];
    uint8_t j0[16];
    uint8_t zero_block[16] = {0};

    if (iv_len != BANK_GCM_IV_BYTES)
        return 0;
    if (!key || !iv || !plain || !cipher || !tag)
        return 0;

    bank_aes256_set_key(&aes_key, key);
    bank_aes_encrypt_block(zero_block, h, &aes_key);

    memset(j0, 0, 16);
    memcpy(j0, iv, iv_len);
    j0[15] = 1;

    bank_gctr(&aes_key, j0, plain, plain_len, cipher);

    uint8_t s[16];
    extern void bank_ghash(uint8_t *h, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t cipher_len, uint8_t *s);
    bank_ghash(h, aad, aad_len, cipher, plain_len, s);

    uint8_t enc_s[16];
    bank_aes_encrypt_block(j0, enc_s, &aes_key);

    for (int i = 0; i < 16; i++)
        tag[i] = s[i] ^ enc_s[i];

    bank_secure_zero(&aes_key, sizeof(aes_key));
    bank_secure_zero(h, sizeof(h));
    bank_secure_zero(j0, sizeof(j0));
    bank_secure_zero(s, sizeof(s));
    bank_secure_zero(enc_s, sizeof(enc_s));

    return 1;
}
