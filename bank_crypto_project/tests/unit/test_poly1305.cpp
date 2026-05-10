#include <stdio.h>
#include <string.h>
#include "bank/bank_poly1305.h"

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

void test_poly1305_basic(void)
{
    bank_poly1305_ctx ctx;
    uint8_t key[32];
    uint8_t tag[16];
    uint8_t expected[16] = {0};

    memset(key, 0, 32);
    memset(expected, 0, 16);

    bank_poly1305_init(&ctx, key);
    bank_poly1305_final(&ctx, tag);

    TEST_ASSERT(memcmp(tag, expected, 16) == 0, "Poly1305 empty input produces zero tag");
}

void test_poly1305_single_block(void)
{
    bank_poly1305_ctx ctx;
    uint8_t key[32];
    uint8_t data[16];
    uint8_t tag[16];

    memset(key, 0, 32);
    memset(data, 0xFF, 16);

    bank_poly1305_init(&ctx, key);
    bank_poly1305_update(&ctx, data, 16);
    bank_poly1305_final(&ctx, tag);

    int non_zero = 0;
    for (int i = 0; i < 16; i++)
        if (tag[i] != 0)
            non_zero = 1;

    TEST_ASSERT(non_zero, "Poly1305 produces non-zero tag for non-empty input");
}

void test_poly1305_multi_block(void)
{
    bank_poly1305_ctx ctx;
    uint8_t key[32];
    uint8_t data[64];
    uint8_t tag1[16];
    uint8_t tag2[16];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 64; i++)
        data[i] = i;

    bank_poly1305_init(&ctx, key);
    bank_poly1305_update(&ctx, data, 64);
    bank_poly1305_final(&ctx, tag1);

    bank_poly1305_init(&ctx, key);
    for (int i = 0; i < 64; i += 16)
        bank_poly1305_update(&ctx, data + i, 16);
    bank_poly1305_final(&ctx, tag2);

    TEST_ASSERT(memcmp(tag1, tag2, 16) == 0, "Poly1305 incremental update matches one-shot");
}

void test_poly1305_with_aad(void)
{
    bank_poly1305_ctx ctx;
    uint8_t key[32];
    uint8_t aad[32];
    uint8_t data[32];
    uint8_t tag[16];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 32; i++)
        aad[i] = i + 0x80;
    for (int i = 0; i < 32; i++)
        data[i] = i + 0x40;

    bank_poly1305_init(&ctx, key);
    bank_poly1305_update(&ctx, aad, 32);
    bank_poly1305_update(&ctx, data, 32);
    bank_poly1305_final(&ctx, tag);

    TEST_ASSERT(tag[0] != 0, "Poly1305 with AAD works");
}

void run_all_poly1305_tests(void)
{
    printf("\n=== Poly1305 Tests ===\n");
    test_poly1305_basic();
    test_poly1305_single_block();
    test_poly1305_multi_block();
    test_poly1305_with_aad();
}
