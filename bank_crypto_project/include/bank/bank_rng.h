#ifndef BANK_RNG_H
#define BANK_RNG_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int bank_random_bytes(uint8_t *buf, size_t len);
    void bank_rng_init(void);
    int bank_rng_bytes(uint8_t *buf, size_t len);
    void bank_rng_reseed(void);
    void bank_rng_add_entropy(const uint8_t *data, size_t len);
    uint64_t bank_rng_entropy_available(void);

#ifdef __cplusplus
}
#endif

#endif
