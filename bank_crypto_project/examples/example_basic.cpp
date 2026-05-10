#include "bank/bank_crypto.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    printf("=== Basic Crypto Example ===\n\n");

    bank_set_log_level(BANK_LOG_INFO);

    if (!bank_init_master_random())
    {
        printf("Failed to initialize crypto module!\n");
        return 1;
    }

    printf("Crypto module initialized successfully\n");

    int session_id = bank_create_session((const uint8_t *)"basic_example", 13, 3600);
    if (session_id <= 0)
    {
        printf("Failed to create session!\n");
        bank_wipe_master();
        return 1;
    }

    printf("Session created: %d\n", session_id);

    const char *plaintext = "Hello, World! This is a secret message.";
    size_t plain_len = strlen(plaintext);
    uint8_t ciphertext[1024];
    uint8_t decrypted[1024];
    uint8_t iv[BANK_GCM_IV_BYTES];
    uint8_t tag[BANK_GCM_TAG_BYTES];
    size_t cipher_len = 0;

    if (bank_encrypt_data((const uint8_t *)plaintext, plain_len, NULL, 0,
                          session_id, ciphertext, &cipher_len, iv, tag))
    {
        printf("Encryption successful! Ciphertext size: %zu bytes\n", cipher_len);
    }
    else
    {
        printf("Encryption failed!\n");
        bank_destroy_session(session_id);
        bank_wipe_master();
        return 1;
    }

    size_t dec_len = 0;
    if (bank_decrypt_data(ciphertext, cipher_len, NULL, 0, session_id,
                          iv, tag, decrypted, &dec_len))
    {
        decrypted[dec_len] = '\0';
        printf("Decryption successful!\n");
        printf("Decrypted text: %s\n", decrypted);
    }
    else
    {
        printf("Decryption failed!\n");
    }

    uint8_t signature[32];
    if (bank_sign_data(session_id, (const uint8_t *)plaintext, plain_len, signature))
    {
        printf("Signature created successfully\n");

        if (bank_verify_signature(session_id, (const uint8_t *)plaintext, plain_len, signature))
        {
            printf("Signature verified successfully\n");
        }
        else
        {
            printf("Signature verification failed!\n");
        }
    }

    bank_destroy_session(session_id);
    bank_wipe_master();

    printf("\n=== Example Completed ===\n");
    return 0;
}
