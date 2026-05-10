#include <stdio.h>
#include <string.h>
#include "bank/bank_gcm.h"

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

void test_gcm_encrypt_decrypt(void)
{
    uint8_t key[32];
    uint8_t iv[12];
    uint8_t plain[64];
    uint8_t cipher[64];
    uint8_t decrypted[64];
    uint8_t tag[16];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 12; i++)
        iv[i] = i;
    for (int i = 0; i < 64; i++)
        plain[i] = i;

    int result = bank_aes256_gcm_encrypt(key, iv, 12, NULL, 0, plain, 64, cipher, tag);
    TEST_ASSERT(result == 1, "AES-GCM encryption succeeds");

    result = bank_aes256_gcm_decrypt(key, iv, 12, NULL, 0, cipher, 64, tag, decrypted);
    TEST_ASSERT(result == 1, "AES-GCM decryption succeeds");

    TEST_ASSERT(memcmp(plain, decrypted, 64) == 0, "AES-GCM roundtrip integrity");
}

void test_gcm_with_aad(void)
{
    uint8_t key[32];
    uint8_t iv[12];
    uint8_t aad[32];
    uint8_t plain[64];
    uint8_t cipher[64];
    uint8_t decrypted[64];
    uint8_t tag[16];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 12; i++)
        iv[i] = i;
    for (int i = 0; i < 32; i++)
        aad[i] = i + 0x80;
    for (int i = 0; i < 64; i++)
        plain[i] = i;

    int result = bank_aes256_gcm_encrypt(key, iv, 12, aad, 32, plain, 64, cipher, tag);
    TEST_ASSERT(result == 1, "AES-GCM encryption with AAD succeeds");

    result = bank_aes256_gcm_decrypt(key, iv, 12, aad, 32, cipher, 64, tag, decrypted);
    TEST_ASSERT(result == 1, "AES-GCM decryption with AAD succeeds");

    TEST_ASSERT(memcmp(plain, decrypted, 64) == 0, "AES-GCM with AAD integrity");
}

void test_gcm_wrong_tag(void)
{
    uint8_t key[32];
    uint8_t iv[12];
    uint8_t plain[64];
    uint8_t cipher[64];
    uint8_t decrypted[64];
    uint8_t tag[16];
    uint8_t wrong_tag[16];

    for (int i = 0; i < 32; i++)
        key[i] = i;
    for (int i = 0; i < 12; i++)
        iv[i] = i;
    for (int i = 0; i < 64; i++)
        plain[i] = i;

    bank_aes256_gcm_encrypt(key, iv, 12, NULL, 0, plain, 64, cipher, tag);

    memset(wrong_tag, 0xFF, 16);
    int result = bank_aes256_gcm_decrypt(key, iv, 12, NULL, 0, cipher, 64, wrong_tag, decrypted);

    TEST_ASSERT(result == 0, "AES-GCM rejects wrong tag");
}

void run_all_gcm_tests(void)
{
    printf("\n=== GCM Tests ===\n");
    test_gcm_encrypt_decrypt();
    test_gcm_with_aad();
    test_gcm_wrong_tag();
}
