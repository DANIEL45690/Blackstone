#include "crypto_api.h"
#include "test_vectors.h"
#include <stdio.h>
#include <string.h>

static CryptoContext test_ctx;

int run_all_tests(void)
{
    int passed = 0;
    int total = 0;
    byte buffer[256];
    byte buffer2[256];

    if (crypto_init(&test_ctx) != CRYPTO_SUCCESS)
    {
        printf("Init failed\n");
        return 0;
    }

    total++;
    if (crypto_self_test(&test_ctx) == CRYPTO_SUCCESS)
    {
        printf("Self test: PASS\n");
        passed++;
    }
    else
    {
        printf("Self test: FAIL\n");
    }

    total++;
    size_t enc_len = binary_encode(&test_ctx, test_vector_1, 16, buffer);
    size_t dec_len = binary_decode(&test_ctx, buffer, enc_len, buffer2);
    if (dec_len == 16 && memcmp(test_vector_1, buffer2, 16) == 0)
    {
        printf("Binary encode/decode: PASS\n");
        passed++;
    }
    else
    {
        printf("Binary encode/decode: FAIL\n");
    }

    total++;
    enc_len = ternary_encode(&test_ctx, test_vector_1, 16, buffer);
    dec_len = ternary_decode(&test_ctx, buffer, enc_len, buffer2);
    if (dec_len == 16 && memcmp(test_vector_1, buffer2, 16) == 0)
    {
        printf("Ternary encode/decode: PASS\n");
        passed++;
    }
    else
    {
        printf("Ternary encode/decode: FAIL\n");
    }

    total++;
    byte hash[HASH_SIZE];
    if (compute_hash_secure(&test_ctx, test_vector_1, 16, hash) == CRYPTO_SUCCESS)
    {
        printf("Hash computation: PASS\n");
        passed++;
    }
    else
    {
        printf("Hash computation: FAIL\n");
    }

    total++;
    byte key[32];
    if (generate_random_key(&test_ctx, key, 32) == CRYPTO_SUCCESS)
    {
        printf("Random key gen: PASS\n");
        passed++;
    }
    else
    {
        printf("Random key gen: FAIL\n");
    }

    total++;
    byte plain[64] = {0};
    byte cipher[64];
    byte result[64];
    if (encrypt_data(&test_ctx, plain, 64, key, 32, NULL, cipher) == CRYPTO_SUCCESS &&
        decrypt_data(&test_ctx, cipher, 64, key, 32, NULL, result) == CRYPTO_SUCCESS &&
        memcmp(plain, result, 64) == 0)
    {
        printf("Encrypt/Decrypt: PASS\n");
        passed++;
    }
    else
    {
        printf("Encrypt/Decrypt: FAIL\n");
    }

    crypto_shutdown(&test_ctx);

    printf("\nTests: %d/%d passed\n", passed, total);
    return (passed == total) ? 1 : 0;
}

int main(void)
{
    return run_all_tests() ? 0 : 1;
}
