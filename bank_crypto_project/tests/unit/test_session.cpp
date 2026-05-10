#include <stdio.h>
#include <string.h>
#include "bank/bank_session.h"
#include "bank/bank_crypto.h"

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

void test_session_create_destroy(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"test_key", 8, 3600);
    TEST_ASSERT(session_id > 0, "Session created successfully");

    bank_destroy_session(session_id);
    TEST_ASSERT(!bank_session_exists(session_id), "Session destroyed successfully");

    bank_wipe_master();
}

void test_session_expiry(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"expiring_key", 12, 1);
    TEST_ASSERT(session_id > 0, "Session with short TTL created");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void test_session_key_retrieval(void)
{
    uint8_t session_key[32];
    uint8_t hmac_key[32];

    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"key_test", 8, 3600);
    TEST_ASSERT(session_id > 0, "Session created");

    int result = bank_get_session_key(session_id, session_key, hmac_key);
    TEST_ASSERT(result == 1, "Session keys retrieved");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void run_all_session_tests(void)
{
    printf("\n=== Session Tests ===\n");
    test_session_create_destroy();
    test_session_expiry();
    test_session_key_retrieval();
}
