#ifndef BANK_HKDF_H
#define BANK_HKDF_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_hkdf_extract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk);
    void bank_hkdf_expand(const uint8_t *prk, size_t prk_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len);
    void bank_hkdf(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len,
                   const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len);

#ifdef __cplusplus
}
#endif

#endif
