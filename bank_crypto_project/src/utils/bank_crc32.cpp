#include "bank/bank_utils.h"
#include <stddef.h>

uint32_t bank_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t poly = 0xEDB88320;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
        }
    }

    return ~crc;
}

uint32_t bank_crc32_combine(uint32_t crc1, uint32_t crc2, size_t len2)
{
    uint32_t crc = crc1;
    uint32_t power = 1;

    for (size_t i = 0; i < len2; i++)
    {
        power = (power << 1) ^ ((power & 0x80000000) ? 0xEDB88320 : 0);
    }

    for (size_t i = 0; i < 32; i++)
    {
        if (power & (1 << i))
        {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }

    return crc ^ crc2;
}
