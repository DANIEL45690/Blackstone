#ifndef BANK_AES_H
#define BANK_AES_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_aes256_set_key(bank_aes256_key *key, const uint8_t *user_key);
    void bank_aes_encrypt_block(const uint8_t *input, uint8_t *output, const bank_aes256_key *key);
    void bank_aes_decrypt_block(const uint8_t *input, uint8_t *output, const bank_aes256_key *key);
    void bank_aes_key_expansion(const uint8_t *key, uint32_t *round_keys, int key_bits);
    void bank_aes_sub_bytes(uint8_t *state);
    void bank_aes_shift_rows(uint8_t *state);
    void bank_aes_mix_columns(uint8_t *state);
    void bank_aes_inv_shift_rows(uint8_t *state);
    void bank_aes_inv_sub_bytes(uint8_t *state);
    void bank_aes_inv_mix_columns(uint8_t *state);
    void bank_aes_add_round_key(uint8_t *state, const uint32_t *key, int round);
    uint8_t bank_xtime(uint8_t x);

    extern const uint8_t bank_sbox[256];
    extern const uint8_t bank_inv_sbox[256];

#ifdef __cplusplus
}
#endif

#endif
