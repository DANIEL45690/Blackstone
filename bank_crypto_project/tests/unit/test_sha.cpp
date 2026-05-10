#include <stdio.h>
#include <string.h>
#include "bank/bank_sha.h"

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

void test_sha256_empty(void)
{
    uint8_t hash[32];
    uint8_t expected[32] = {
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
        0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
        0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

    bank_sha256(NULL, 0, hash);
    TEST_ASSERT(memcmp(hash, expected, 32) == 0, "SHA-256 empty string");
}

void test_sha256_hello(void)
{
    uint8_t hash[32];
    const char *input = "hello";
    uint8_t expected[32] = {
        0x2c, 0xf2, 0x4d, 0xba, 0x5f, 0xb0, 0xa3, 0x0e,
        0x26, 0x83, 0x12, 0x1d, 0xcf, 0x84, 0x01, 0x64,
        0xa7, 0xfb, 0xe1, 0x55, 0x8d, 0x0c, 0xb3, 0x2d,
        0x80, 0x04, 0xc4, 0x9b, 0x5e, 0x5a, 0x21, 0xb1};

    bank_sha256((const uint8_t *)input, 5, hash);
    TEST_ASSERT(memcmp(hash, expected, 32) == 0, "SHA-256 'hello'");
}

void test_sha256_long(void)
{
    uint8_t hash[32];
    uint8_t input[1000];

    for (int i = 0; i < 1000; i++)
        input[i] = i & 0xFF;

    bank_sha256(input, 1000, hash);

    TEST_ASSERT(hash[0] != 0, "SHA-256 long input produces non-zero hash");
}

void test_sha512_empty(void)
{
    uint8_t hash[64];
    bank_sha512(NULL, 0, hash);
    TEST_ASSERT(hash[0] != 0, "SHA-512 empty string produces hash");
}

void test_sha512_hello(void)
{
    uint8_t hash[64];
    const char *input = "hello";

    bank_sha512((const uint8_t *)input, 5, hash);
    TEST_ASSERT(hash[0] != 0, "SHA-512 produces non-zero hash");
}

void test_sha256_update_final(void)
{
    bank_sha256_ctx ctx;
    uint8_t hash1[32];
    uint8_t hash2[32];
    const char *input = "Hello World";

    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, (const uint8_t *)input, 5);
    bank_sha256_update(&ctx, (const uint8_t *)(input + 5), 6);
    bank_sha256_final(&ctx, hash1);

    bank_sha256((const uint8_t *)input, 11, hash2);

    TEST_ASSERT(memcmp(hash1, hash2, 32) == 0, "SHA-256 incremental update matches one-shot");
}

void run_all_sha_tests(void)
{
    printf("\n=== SHA Tests ===\n");
    test_sha256_empty();
    test_sha256_hello();
    test_sha256_long();
    test_sha512_empty();
    test_sha512_hello();
    test_sha256_update_final();
}
