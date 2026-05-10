#ifndef BANK_INTERNAL_GCM_H
#define BANK_INTERNAL_GCM_H

#include "../bank_types.h"
#include "../bank_aes.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_gcm_multiply(uint8_t *x, const uint8_t *h);
    void bank_gcm_precompute_h(const bank_aes256_key *key, uint8_t *h);
    void bank_gcm_init_ctr(uint8_t *j0, const uint8_t *iv, size_t iv_len);
    void bank_gcm_increment_ctr(uint8_t *ctr);
    void bank_gcm_ghash_final(uint8_t *x, const uint8_t *aad, size_t aad_len,
                              const uint8_t *ciphertext, size_t cipher_len,
                              uint8_t *s);
    void bank_gcm_gctr_crypt(const bank_aes256_key *key, const uint8_t *icb,
                             const uint8_t *input, size_t input_len, uint8_t *output);
    int bank_gcm_verify_tag(const uint8_t *tag1, const uint8_t *tag2, size_t len);

#ifdef __cplusplus
}
#endif

#endif
