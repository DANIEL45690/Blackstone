#include <stdio.h>
#include <string.h>
#include "bank/bank_chacha20.h"

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

void test_chacha20_basic(void)
{
    bank_chacha20_ctx ctx;
    uint8_t key[32];
    uint8_t nonce[12];
    uint8_t plain[64];
    uint8_t cipher[64];
    uint8_t decrypted[64];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 12; i++)
        nonce[i] = i;
    for (int i = 0; i < 64; i++)
        plain[i] = i;

    bank_chacha20_init(&ctx, key, nonce, 0);
    bank_chacha20_encrypt(&ctx, plain, 64, cipher);

    bank_chacha20_init(&ctx, key, nonce, 0);
    bank_chacha20_encrypt(&ctx, cipher, 64, decrypted);

    TEST_ASSERT(memcmp(plain, decrypted, 64) == 0, "ChaCha20 encrypt/decrypt roundtrip");
}

void test_chacha20_block(void)
{
    bank_chacha20_ctx ctx;
    uint8_t key[32] = {0};
    uint8_t nonce[12] = {0};
    uint8_t block[64];

    bank_chacha20_init(&ctx, key, nonce, 0);
    bank_chacha20_block(&ctx, block);

    int non_zero = 0;
    for (int i = 0; i < 64; i++)
        if (block[i] != 0)
            non_zero = 1;

    TEST_ASSERT(non_zero, "ChaCha20 block produces non-zero output");
}

void test_chacha20_counter(void)
{
    bank_chacha20_ctx ctx;
    uint8_t key[32] = {0};
    uint8_t nonce[12] = {0};
    uint8_t block1[64];
    uint8_t block2[64];

    bank_chacha20_init(&ctx, key, nonce, 0);
    bank_chacha20_block(&ctx, block1);
    bank_chacha20_block(&ctx, block2);

    TEST_ASSERT(bank_chacha20_get_counter(&ctx) == 2, "ChaCha20 counter increments");
    TEST_ASSERT(memcmp(block1, block2, 64) != 0, "ChaCha20 different blocks are different");
}

void test_chacha20_set_counter(void)
{
    bank_chacha20_ctx ctx;
    uint8_t key[32] = {0};
    uint8_t nonce[12] = {0};
    uint8_t block1[64];
    uint8_t block2[64];

    bank_chacha20_init(&ctx, key, nonce, 5);
    bank_chacha20_block(&ctx, block1);

    bank_chacha20_init(&ctx, key, nonce, 0);
    bank_chacha20_set_counter(&ctx, 5);
    bank_chacha20_block(&ctx, block2);

    TEST_ASSERT(memcmp(block1, block2, 64) == 0, "ChaCha20 set counter works");
}

void run_all_chacha_tests(void)
{
    printf("\n=== ChaCha20 Tests ===\n");
    test_chacha20_basic();
    test_chacha20_block();
    test_chacha20_counter();
    test_chacha20_set_counter();
}
