#include <stdio.h>
#include <string.h>
#include "bank/bank_pbkdf2.h"

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

void test_pbkdf2_basic(void)
{
    uint8_t key[32];
    const uint8_t password[] = "password";
    const uint8_t salt[] = "salt";

    bank_pbkdf2_hmac_sha256(password, 8, salt, 4, 1, key, 32);

    TEST_ASSERT(key[0] != 0, "PBKDF2 produces non-zero key");
}

void test_pbkdf2_different_iterations(void)
{
    uint8_t key1[32];
    uint8_t key2[32];
    const uint8_t password[] = "password";
    const uint8_t salt[] = "salt";

    bank_pbkdf2_hmac_sha256(password, 8, salt, 4, 1, key1, 32);
    bank_pbkdf2_hmac_sha256(password, 8, salt, 4, 2, key2, 32);

    TEST_ASSERT(memcmp(key1, key2, 32) != 0, "PBKDF2 different iterations produce different keys");
}

void test_pbkdf2_different_salt(void)
{
    uint8_t key1[32];
    uint8_t key2[32];
    const uint8_t password[] = "password";
    const uint8_t salt1[] = "salt1";
    const uint8_t salt2[] = "salt2";

    bank_pbkdf2_hmac_sha256(password, 8, salt1, 5, 1000, key1, 32);
    bank_pbkdf2_hmac_sha256(password, 8, salt2, 5, 1000, key2, 32);

    TEST_ASSERT(memcmp(key1, key2, 32) != 0, "PBKDF2 different salts produce different keys");
}

void test_derive_key_from_password(void)
{
    uint8_t key[32];
    const uint8_t password[] = "my_secure_password";
    const uint8_t salt[] = "random_salt_12345";

    int result = bank_derive_key_from_password(password, 19, salt, 16, key, 32);

    TEST_ASSERT(result == 1, "bank_derive_key_from_password succeeds");
    TEST_ASSERT(key[0] != 0, "Derived key is non-zero");
}

void run_all_pbkdf2_tests(void)
{
    printf("\n=== PBKDF2 Tests ===\n");
    test_pbkdf2_basic();
    test_pbkdf2_different_iterations();
    test_pbkdf2_different_salt();
    test_derive_key_from_password();
}
