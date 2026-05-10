#include <stdio.h>
#include <string.h>
#include "bank/bank_rng.h"

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

void test_random_bytes(void)
{
    uint8_t buf1[64];
    uint8_t buf2[64];

    memset(buf1, 0, 64);
    memset(buf2, 0, 64);

    int result = bank_random_bytes(buf1, 64);
    TEST_ASSERT(result == 1, "bank_random_bytes succeeds");

    result = bank_random_bytes(buf2, 64);
    TEST_ASSERT(result == 1, "bank_random_bytes second call succeeds");

    TEST_ASSERT(memcmp(buf1, buf2, 64) != 0, "Random bytes are different");
}

void test_random_bytes_different_sizes(void)
{
    uint8_t buf1[16];
    uint8_t buf2[32];
    uint8_t buf3[128];

    TEST_ASSERT(bank_random_bytes(buf1, 16) == 1, "Random 16 bytes works");
    TEST_ASSERT(bank_random_bytes(buf2, 32) == 1, "Random 32 bytes works");
    TEST_ASSERT(bank_random_bytes(buf3, 128) == 1, "Random 128 bytes works");
}

void test_rng_entropy(void)
{
    bank_rng_init();

    uint64_t entropy = bank_rng_entropy_available();
    TEST_ASSERT(entropy > 0, "RNG has entropy after init");
}

void run_all_rng_tests(void)
{
    printf("\n=== RNG Tests ===\n");
    test_random_bytes();
    test_random_bytes_different_sizes();
    test_rng_entropy();
}
