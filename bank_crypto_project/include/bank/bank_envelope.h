#ifndef BANK_ENVELOPE_H
#define BANK_ENVELOPE_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int bank_create_envelope(const uint8_t *plain, size_t plain_len,
                             const uint8_t *aad, size_t aad_len,
                             int session_id, bank_secure_envelope *envelope);

    int bank_extract_envelope(const bank_secure_envelope *envelope,
                              const uint8_t *aad, size_t aad_len,
                              uint8_t *plain, size_t *plain_len);

    int bank_verify_envelope(const bank_secure_envelope *envelope, const uint8_t *aad, size_t aad_len);
    int bank_envelope_get_version(const bank_secure_envelope *envelope);
    uint64_t bank_envelope_get_timestamp(const bank_secure_envelope *envelope);

#ifdef __cplusplus
}
#endif

#endif
