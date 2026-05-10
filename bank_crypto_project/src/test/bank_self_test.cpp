#include "bank/bank_crypto.h"
#include "bank/bank_utils.h"
#include "bank/bank_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bank_self_test(void)
{
    uint8_t key[32];
    uint8_t iv[12];
    uint8_t plain[128];
    uint8_t cipher[128];
    uint8_t dec[128];
    uint8_t tag[16];
    uint8_t signature[32];
    bank_transaction tx;
    bank_secure_envelope env;
    int session_id;
    size_t out_len;

    for (int i = 0; i < 32; i++)
        key[i] = i & 0xFF;
    for (int i = 0; i < 12; i++)
        iv[i] = i & 0xFF;
    for (int i = 0; i < 128; i++)
        plain[i] = i & 0xFF;

    if (!bank_aes256_gcm_encrypt(key, iv, 12, NULL, 0, plain, 128, cipher, tag))
    {
        BANK_LOG_FATAL("AES-GCM encrypt self-test failed");
        exit(1);
    }

    if (!bank_aes256_gcm_decrypt(key, iv, 12, NULL, 0, cipher, 128, tag, dec))
    {
        BANK_LOG_FATAL("AES-GCM decrypt self-test failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG_FATAL("AES-GCM integrity self-test failed");
        exit(1);
    }

    uint8_t hash[32];
    bank_sha256(plain, 128, hash);

    bank_hmac_sha256(key, 32, plain, 128, signature);

    uint8_t expected_sig[32];
    bank_hmac_sha256(key, 32, plain, 128, expected_sig);
    if (memcmp(signature, expected_sig, 32) != 0)
    {
        BANK_LOG_FATAL("HMAC-SHA256 self-test failed");
        exit(1);
    }

    uint8_t chacha_key[32];
    uint8_t chacha_nonce[12];
    uint8_t chacha_plain[64];
    uint8_t chacha_cipher[64];
    uint8_t chacha_dec[64];

    for (int i = 0; i < 32; i++)
        chacha_key[i] = i;
    for (int i = 0; i < 12; i++)
        chacha_nonce[i] = i;
    for (int i = 0; i < 64; i++)
        chacha_plain[i] = i;

    bank_chacha20_ctx chacha_ctx;
    memcpy(chacha_ctx.key, chacha_key, 32);
    memcpy(chacha_ctx.nonce, chacha_nonce, 12);
    chacha_ctx.counter = 0;
    bank_chacha20_encrypt(&chacha_ctx, chacha_plain, 64, chacha_cipher);

    memcpy(chacha_ctx.key, chacha_key, 32);
    memcpy(chacha_ctx.nonce, chacha_nonce, 12);
    chacha_ctx.counter = 0;
    bank_chacha20_encrypt(&chacha_ctx, chacha_cipher, 64, chacha_dec);

    if (memcmp(chacha_plain, chacha_dec, 64) != 0)
    {
        BANK_LOG_FATAL("ChaCha20 self-test failed");
        exit(1);
    }

    uint8_t poly_tag[BANK_POLY1305_TAG_SIZE];
    if (!bank_encrypt_chacha20_poly1305(key, iv, NULL, 0, plain, 128, cipher, poly_tag))
    {
        BANK_LOG_FATAL("ChaCha20-Poly1305 encrypt self-test failed");
        exit(1);
    }

    if (!bank_init_master_random())
    {
        BANK_LOG_FATAL("Master key init failed");
        exit(1);
    }

    session_id = bank_create_session((const uint8_t *)"test_key_self_test", 19, 3600);
    if (session_id <= 0)
    {
        BANK_LOG_FATAL("Session creation failed");
        exit(1);
    }

    if (!bank_encrypt_data(plain, 128, NULL, 0, session_id, cipher, &out_len, iv, tag))
    {
        BANK_LOG_FATAL("Bank encrypt failed");
        exit(1);
    }

    if (!bank_decrypt_data(cipher, out_len, NULL, 0, session_id, iv, tag, dec, &out_len))
    {
        BANK_LOG_FATAL("Bank decrypt failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG_FATAL("Bank encrypt/decrypt integrity failed");
        exit(1);
    }

    if (!bank_sign_data(session_id, plain, 128, signature))
    {
        BANK_LOG_FATAL("Signing failed");
        exit(1);
    }

    if (!bank_verify_signature(session_id, plain, 128, signature))
    {
        BANK_LOG_FATAL("Signature verification failed");
        exit(1);
    }

    if (!bank_create_transaction(12345678, plain, plain + 32, 1000000, (const uint8_t *)"USD", session_id, &tx))
    {
        BANK_LOG_FATAL("Transaction creation failed");
        exit(1);
    }

    if (!bank_verify_transaction(&tx, session_id))
    {
        BANK_LOG_FATAL("Transaction verification failed");
        exit(1);
    }

    if (!bank_create_envelope(plain, 128, NULL, 0, session_id, &env))
    {
        BANK_LOG_FATAL("Envelope creation failed");
        exit(1);
    }

    if (!bank_extract_envelope(&env, NULL, 0, dec, &out_len))
    {
        BANK_LOG_FATAL("Envelope extraction failed");
        exit(1);
    }

    if (memcmp(plain, dec, 128) != 0)
    {
        BANK_LOG_FATAL("Envelope integrity failed");
        exit(1);
    }

    uint8_t derived_key[32];
    uint8_t password[] = "test_password";
    uint8_t salt[] = "random_salt_12345";
    if (!bank_derive_key_from_password(password, sizeof(password) - 1, salt, sizeof(salt) - 1, derived_key, 32))
    {
        BANK_LOG_FATAL("Key derivation failed");
        exit(1);
    }

    bank_rotate_master_key();

    int session_id2 = bank_create_session((const uint8_t *)"test_key_after_rotation", 24, 3600);
    if (session_id2 <= 0)
    {
        BANK_LOG_FATAL("Session creation after rotation failed");
        exit(1);
    }

    bank_destroy_session(session_id);
    bank_destroy_session(session_id2);
    bank_wipe_master();

    BANK_LOG_INFO("All self-tests passed successfully");
    fprintf(stderr, "All self-tests passed successfully\n");
}
