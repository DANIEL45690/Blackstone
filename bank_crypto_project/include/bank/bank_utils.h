#ifndef BANK_UTILS_H
#define BANK_UTILS_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_secure_zero(void *ptr, size_t len);
    int bank_secure_compare(const uint8_t *a, const uint8_t *b, size_t len);
    uint32_t bank_crc32(const uint8_t *data, size_t len);
    uint32_t bank_crc32_combine(uint32_t crc1, uint32_t crc2, size_t len2);
    void bank_memxor(uint8_t *dst, const uint8_t *src, size_t len);
    int bank_is_zero(const uint8_t *data, size_t len);
    void bank_swap_bytes(uint8_t *a, uint8_t *b);
    uint16_t bank_bswap16(uint16_t x);
    uint32_t bank_bswap32(uint32_t x);
    uint64_t bank_bswap64(uint64_t x);

#ifdef __cplusplus
}
#endif

#endif
