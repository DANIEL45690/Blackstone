#include "bank/bank_gcm.h"
#include "bank/bank_aes.h"
#include "bank/bank_utils.h"
#include <string.h>

int bank_aes256_gcm_decrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len,
                            const uint8_t *aad, size_t aad_len,
                            const uint8_t *cipher, size_t cipher_len,
                            const uint8_t *tag, uint8_t *plain)
{
    bank_aes256_key aes_key;
    uint8_t h[16];
    uint8_t j0[16];
    uint8_t zero_block[16] = {0};
    uint8_t s[16];
    uint8_t enc_s[16];
    uint8_t expected_tag[16];

    if (iv_len != BANK_GCM_IV_BYTES)
        return 0;
    if (!key || !iv || !cipher || !tag || !plain)
        return 0;

    bank_aes256_set_key(&aes_key, key);
    bank_aes_encrypt_block(zero_block, h, &aes_key);

    memset(j0, 0, 16);
    memcpy(j0, iv, iv_len);
    j0[15] = 1;

    extern void bank_ghash(uint8_t *h, const uint8_t *aad, size_t aad_len, const uint8_t *ciphertext, size_t cipher_len, uint8_t *s);
    bank_ghash(h, aad, aad_len, cipher, cipher_len, s);
    bank_aes_encrypt_block(j0, enc_s, &aes_key);

    for (int i = 0; i < 16; i++)
        expected_tag[i] = s[i] ^ enc_s[i];

    int tag_ok = 1;
    for (int i = 0; i < 16; i++)
    {
        if (tag[i] != expected_tag[i])
            tag_ok = 0;
    }

    if (tag_ok)
    {
        extern void bank_gctr(const bank_aes256_key *key, const uint8_t *icb, const uint8_t *x, size_t xlen, uint8_t *y);
        bank_gctr(&aes_key, j0, cipher, cipher_len, plain);
    }

    bank_secure_zero(&aes_key, sizeof(aes_key));
    bank_secure_zero(h, sizeof(h));
    bank_secure_zero(j0, sizeof(j0));
    bank_secure_zero(s, sizeof(s));
    bank_secure_zero(enc_s, sizeof(enc_s));
    bank_secure_zero(expected_tag, sizeof(expected_tag));

    return tag_ok;
}
