#include <stdio.h>
#include <stdlib.h>

void run_all_aes_tests(void);
void run_all_sha_tests(void);
void run_all_chacha_tests(void);
void run_all_poly1305_tests(void);
void run_all_gcm_tests(void);
void run_all_hmac_tests(void);
void run_all_pbkdf2_tests(void);
void run_all_hkdf_tests(void);
void run_all_rng_tests(void);
void run_all_session_tests(void);
void run_all_transaction_tests(void);
void run_all_envelope_tests(void);
void run_all_utils_tests(void);

int main(void)
{
    printf("========================================\n");
    printf("    BANK CRYPTO UNIT TESTS\n");
    printf("========================================\n");

    run_all_aes_tests();
    run_all_sha_tests();
    run_all_chacha_tests();
    run_all_poly1305_tests();
    run_all_gcm_tests();
    run_all_hmac_tests();
    run_all_pbkdf2_tests();
    run_all_hkdf_tests();
    run_all_rng_tests();
    run_all_session_tests();
    run_all_transaction_tests();
    run_all_envelope_tests();
    run_all_utils_tests();

    printf("\n========================================\n");
    printf("All tests completed\n");
    printf("========================================\n");

    return 0;
}
