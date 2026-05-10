#ifndef BANK_PBKDF2_H
#define BANK_PBKDF2_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len,
                                 const uint8_t *salt, size_t salt_len,
                                 int iterations, uint8_t *key, size_t key_len);

    int bank_derive_key_from_password(const uint8_t *password, size_t password_len,
                                      const uint8_t *salt, size_t salt_len,
                                      uint8_t *key, size_t key_len);

#ifdef __cplusplus
}
#endif

#endif
