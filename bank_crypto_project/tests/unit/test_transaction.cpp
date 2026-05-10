#include <stdio.h>
#include <string.h>
#include "bank/bank_transaction.h"
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

void test_transaction_create_verify(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"tx_test", 7, 3600);
    TEST_ASSERT(session_id > 0, "Session created");

    uint8_t from[32] = {0};
    uint8_t to[32] = {0};
    memcpy(from, "FROM_ACCOUNT_1", 15);
    memcpy(to, "TO_ACCOUNT_1", 12);

    bank_transaction tx;
    int result = bank_create_transaction(1, from, to, 1000, (const uint8_t *)"USD", session_id, &tx);
    TEST_ASSERT(result == 1, "Transaction created");

    result = bank_verify_transaction(&tx, session_id);
    TEST_ASSERT(result == 1, "Transaction verified");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void test_transaction_metadata(void)
{
    bank_transaction tx;
    uint8_t metadata[] = "Additional transaction data";

    memset(&tx, 0, sizeof(tx));

    bank_transaction_set_metadata(&tx, metadata, sizeof(metadata) - 1);
    TEST_ASSERT(tx.metadata_len == sizeof(metadata) - 1, "Metadata set correctly");
    TEST_ASSERT(memcmp(tx.metadata, metadata, tx.metadata_len) == 0, "Metadata content correct");

    bank_transaction_clear_metadata(&tx);
    TEST_ASSERT(tx.metadata_len == 0, "Metadata cleared");
}

void run_all_transaction_tests(void)
{
    printf("\n=== Transaction Tests ===\n");
    test_transaction_create_verify();
    test_transaction_metadata();
}
