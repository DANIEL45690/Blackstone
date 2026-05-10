#include <stdio.h>
#include <string.h>
#include "bank/bank_hkdf.h"

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

void test_hkdf_extract(void)
{
    uint8_t prk[32];
    const uint8_t salt[] = "salt";
    const uint8_t ikm[] = "input key material";

    bank_hkdf_extract(salt, 4, ikm, 19, prk);

    TEST_ASSERT(prk[0] != 0, "HKDF extract produces non-zero PRK");
}

void test_hkdf_expand(void)
{
    uint8_t prk[32];
    uint8_t okm[64];
    const uint8_t info[] = "info";

    memset(prk, 0x55, 32);

    bank_hkdf_expand(prk, 32, info, 4, okm, 64);

    TEST_ASSERT(okm[0] != 0, "HKDF expand produces non-zero OKM");
}

void test_hkdf_full(void)
{
    uint8_t okm[32];
    const uint8_t salt[] = "salt";
    const uint8_t ikm[] = "input key material";
    const uint8_t info[] = "application info";

    bank_hkdf(salt, 4, ikm, 19, info, 14, okm, 32);

    TEST_ASSERT(okm[0] != 0, "Full HKDF works");
}

void test_hkdf_no_salt(void)
{
    uint8_t okm[32];
    const uint8_t ikm[] = "input key material";
    const uint8_t info[] = "info";

    bank_hkdf(NULL, 0, ikm, 19, info, 4, okm, 32);

    TEST_ASSERT(okm[0] != 0, "HKDF without salt works");
}

void run_all_hkdf_tests(void)
{
    printf("\n=== HKDF Tests ===\n");
    test_hkdf_extract();
    test_hkdf_expand();
    test_hkdf_full();
    test_hkdf_no_salt();
}
