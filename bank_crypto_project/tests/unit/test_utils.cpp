#include <stdio.h>
#include <string.h>
#include "bank/bank_utils.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg)          \
    do                                  \
    {                                   \
        if (cond)                       \
        {                               \
            tests_passed++;             \
            printf("[PASS] %s\n", msg); \
        }                               \
        else                            \
        {                               \
            tests_failed++;             \
            printf("[FAIL] %s\n", msg); \
        }                               \
    } while (0)

void test_secure_zero(void)
{
    uint8_t data[64];
    memset(data, 0x55, 64);

    bank_secure_zero(data, 64);

    for (int i = 0; i < 64; i++)
        TEST_ASSERT(data[i] == 0, "bank_secure_zero clears memory");
}

void test_secure_compare(void)
{
    uint8_t a[32];
    uint8_t b[32];

    memset(a, 0xAA, 32);
    memset(b, 0xAA, 32);

    TEST_ASSERT(bank_secure_compare(a, b, 32) == 1, "bank_secure_compare equal returns true");

    b[16] = 0xBB;
    TEST_ASSERT(bank_secure_compare(a, b, 32) == 0, "bank_secure_compare different returns false");
}

void test_crc32(void)
{
    const uint8_t data[] = "Hello, World!";
    uint32_t crc = bank_crc32(data, sizeof(data) - 1);

    TEST_ASSERT(crc != 0, "CRC32 produces non-zero result");

    uint32_t crc2 = bank_crc32(data, sizeof(data) - 1);
    TEST_ASSERT(crc == crc2, "CRC32 is deterministic");
}

void test_crc32_combine(void)
{
    const uint8_t data1[] = "Hello, ";
    const uint8_t data2[] = "World!";

    uint32_t crc1 = bank_crc32(data1, sizeof(data1) - 1);
    uint32_t crc2 = bank_crc32(data2, sizeof(data2) - 1);
    uint32_t crc_combined = bank_crc32_combine(crc1, crc2, sizeof(data2) - 1);

    uint32_t crc_full = bank_crc32((const uint8_t *)"Hello, World!", 13);

    TEST_ASSERT(crc_combined == crc_full, "CRC32 combine matches full CRC");
}

void test_memxor(void)
{
    uint8_t dst[16];
    uint8_t src[16];

    memset(dst, 0xAA, 16);
    memset(src, 0xFF, 16);

    bank_memxor(dst, src, 16);

    for (int i = 0; i < 16; i++)
        TEST_ASSERT(dst[i] == 0x55, "memxor works correctly");
}

void test_is_zero(void)
{
    uint8_t zero[32] = {0};
    uint8_t non_zero[32];

    memset(non_zero, 0, 32);
    non_zero[10] = 1;

    TEST_ASSERT(bank_is_zero(zero, 32) == 1, "bank_is_zero detects all zeros");
    TEST_ASSERT(bank_is_zero(non_zero, 32) == 0, "bank_is_zero detects non-zero");
}

void test_byte_swap(void)
{
    uint16_t x16 = 0x1234;
    uint32_t x32 = 0x12345678;
    uint64_t x64 = 0x123456789ABCDEF0ULL;

    TEST_ASSERT(bank_bswap16(x16) == 0x3412, "bswap16 works");
    TEST_ASSERT(bank_bswap32(x32) == 0x78563412, "bswap32 works");
    TEST_ASSERT(bank_bswap64(x64) == 0xF0DEBC9A78563412ULL, "bswap64 works");
}

void run_all_utils_tests(void)
{
    printf("\n=== Utils Tests ===\n");
    test_secure_zero();
    test_secure_compare();
    test_crc32();
    test_crc32_combine();
    test_memxor();
    test_is_zero();
    test_byte_swap();
}
