#include <stdio.h>
#include <string.h>
#include "bank/bank_hmac.h"

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

void test_hmac_basic(void)
{
    uint8_t key[32];
    uint8_t data[64];
    uint8_t mac1[32];
    uint8_t mac2[32];

    memset(key, 0, 32);
    memset(data, 0, 64);

    bank_hmac_sha256(key, 32, data, 64, mac1);
    bank_hmac_sha256(key, 32, data, 64, mac2);

    TEST_ASSERT(memcmp(mac1, mac2, 32) == 0, "HMAC deterministic");
}

void test_hmac_different_key(void)
{
    uint8_t key1[32];
    uint8_t key2[32];
    uint8_t data[64];
    uint8_t mac1[32];
    uint8_t mac2[32];

    memset(key1, 0, 32);
    memset(key2, 1, 32);
    memset(data, 0, 64);

    bank_hmac_sha256(key1, 32, data, 64, mac1);
    bank_hmac_sha256(key2, 32, data, 64, mac2);

    TEST_ASSERT(memcmp(mac1, mac2, 32) != 0, "HMAC different keys produce different tags");
}

void test_hmac_different_data(void)
{
    uint8_t key[32];
    uint8_t data1[64];
    uint8_t data2[64];
    uint8_t mac1[32];
    uint8_t mac2[32];

    memset(key, 0, 32);
    memset(data1, 0, 64);
    memset(data2, 1, 64);

    bank_hmac_sha256(key, 32, data1, 64, mac1);
    bank_hmac_sha256(key, 32, data2, 64, mac2);

    TEST_ASSERT(memcmp(mac1, mac2, 32) != 0, "HMAC different data produce different tags");
}

void test_hmac_long_key(void)
{
    uint8_t key[100];
    uint8_t data[64];
    uint8_t mac[32];

    for (int i = 0; i < 100; i++)
        key[i] = i;
    memset(data, 0, 64);

    bank_hmac_sha256(key, 100, data, 64, mac);

    TEST_ASSERT(mac[0] != 0, "HMAC long key works");
}

void run_all_hmac_tests(void)
{
    printf("\n=== HMAC Tests ===\n");
    test_hmac_basic();
    test_hmac_different_key();
    test_hmac_different_data();
    test_hmac_long_key();
}
