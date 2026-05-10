#include <stdio.h>
#include <string.h>
#include "bank/bank_envelope.h"
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

void test_envelope_create_extract(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"env_test", 8, 3600);
    TEST_ASSERT(session_id > 0, "Session created");

    const uint8_t plain[] = "Secret message for envelope";
    size_t plain_len = sizeof(plain);

    bank_secure_envelope envelope;
    int result = bank_create_envelope(plain, plain_len, NULL, 0, session_id, &envelope);
    TEST_ASSERT(result == 1, "Envelope created");

    uint8_t extracted[256];
    size_t extracted_len = 0;
    result = bank_extract_envelope(&envelope, NULL, 0, extracted, &extracted_len);
    TEST_ASSERT(result == 1, "Envelope extracted");
    TEST_ASSERT(extracted_len == plain_len, "Extracted length matches");
    TEST_ASSERT(memcmp(plain, extracted, plain_len) == 0, "Extracted content matches");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void test_envelope_with_aad(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"env_aad", 7, 3600);

    const uint8_t plain[] = "Secret data";
    const uint8_t aad[] = "Additional authenticated data";

    bank_secure_envelope envelope;
    int result = bank_create_envelope(plain, sizeof(plain), aad, sizeof(aad), session_id, &envelope);
    TEST_ASSERT(result == 1, "Envelope with AAD created");

    uint8_t extracted[256];
    size_t extracted_len;
    result = bank_extract_envelope(&envelope, aad, sizeof(aad), extracted, &extracted_len);
    TEST_ASSERT(result == 1, "Envelope with AAD extracted");

    result = bank_extract_envelope(&envelope, NULL, 0, extracted, &extracted_len);
    TEST_ASSERT(result == 0, "Envelope extraction fails with wrong AAD");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void test_envelope_version_timestamp(void)
{
    bank_init_master_random();

    int session_id = bank_create_session((const uint8_t *)"env_info", 8, 3600);

    bank_secure_envelope envelope;
    bank_create_envelope((const uint8_t *)"test", 4, NULL, 0, session_id, &envelope);

    TEST_ASSERT(bank_envelope_get_version(&envelope) == 2, "Envelope version correct");
    TEST_ASSERT(bank_envelope_get_timestamp(&envelope) > 0, "Envelope timestamp non-zero");

    bank_destroy_session(session_id);
    bank_wipe_master();
}

void run_all_envelope_tests(void)
{
    printf("\n=== Envelope Tests ===\n");
    test_envelope_create_extract();
    test_envelope_with_aad();
    test_envelope_version_timestamp();
}
