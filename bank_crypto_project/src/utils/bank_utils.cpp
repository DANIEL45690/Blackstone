#include "bank/bank_utils.h"
#include <string.h>

void bank_secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    while (len--)
        *p++ = 0;
}

int bank_secure_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    volatile uint8_t result = 0;

    bank_platform_sleep_ms(BANK_SECURE_COMPARE_DELAY_MS);

    for (size_t i = 0; i < len; i++)
    {
        result |= a[i] ^ b[i];
    }

    bank_platform_sleep_ms(BANK_SECURE_COMPARE_DELAY_MS);

    return result == 0;
}

void bank_memxor(uint8_t *dst, const uint8_t *src, size_t len)
{
    for (size_t i = 0; i < len; i++)
        dst[i] ^= src[i];
}

int bank_is_zero(const uint8_t *data, size_t len)
{
    uint8_t result = 0;
    for (size_t i = 0; i < len; i++)
        result |= data[i];
    return result == 0;
}

void bank_swap_bytes(uint8_t *a, uint8_t *b)
{
    uint8_t t = *a;
    *a = *b;
    *b = t;
}

uint16_t bank_bswap16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}

uint32_t bank_bswap32(uint32_t x)
{
    return (x >> 24) |
           ((x >> 8) & 0x0000FF00) |
           ((x << 8) & 0x00FF0000) |
           (x << 24);
}

uint64_t bank_bswap64(uint64_t x)
{
    return (x >> 56) |
           ((x >> 40) & 0x000000000000FF00ULL) |
           ((x >> 24) & 0x0000000000FF0000ULL) |
           ((x >> 8) & 0x00000000FF000000ULL) |
           ((x << 8) & 0x000000FF00000000ULL) |
           ((x << 24) & 0x0000FF0000000000ULL) |
           ((x << 40) & 0x00FF000000000000ULL) |
           (x << 56);
}
