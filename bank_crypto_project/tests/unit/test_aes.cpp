#include <stdio.h>
#include <string.h>
#include "bank/bank_aes.h"
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

void test_aes_encrypt_decrypt(void)
{
    bank_aes256_key key;
    uint8_t user_key[32];
    uint8_t plain[16];
    uint8_t cipher[16];
    uint8_t decrypted[16];

    for (int i = 0; i < 32; i++)
        user_key[i] = i;
    for (int i = 0; i < 16; i++)
        plain[i] = i;

    bank_aes256_set_key(&key, user_key);
    bank_aes_encrypt_block(plain, cipher, &key);
    bank_aes_decrypt_block(cipher, decrypted, &key);

    TEST_ASSERT(memcmp(plain, decrypted, 16) == 0, "AES-256 encrypt/decrypt roundtrip");
}

void test_aes_key_expansion(void)
{
    uint32_t round_keys[60];
    uint8_t key[32];

    for (int i = 0; i < 32; i++)
        key[i] = i;

    bank_aes_key_expansion(key, round_keys, 256);

    TEST_ASSERT(round_keys[0] != 0, "AES key expansion produces non-zero keys");
    TEST_ASSERT(round_keys[59] != 0, "AES all round keys generated");
}

void test_aes_sub_bytes(void)
{
    uint8_t state[16];
    for (int i = 0; i < 16; i++)
        state[i] = i;

    bank_aes_sub_bytes(state);

    TEST_ASSERT(state[0] == 0x63, "AES SubBytes works correctly");
}

void test_aes_shift_rows(void)
{
    uint8_t state[16];
    for (int i = 0; i < 16; i++)
        state[i] = i;

    bank_aes_shift_rows(state);

    TEST_ASSERT(state[1] == 5, "AES ShiftRows works correctly");
}

void test_aes_mix_columns(void)
{
    uint8_t state[16] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

    bank_aes_mix_columns(state);

    TEST_ASSERT(state[0] != 0x01, "AES MixColumns transforms state");
}

void run_all_aes_tests(void)
{
    printf("\n=== AES Tests ===\n");
    test_aes_encrypt_decrypt();
    test_aes_key_expansion();
    test_aes_sub_bytes();
    test_aes_shift_rows();
    test_aes_mix_columns();
}
