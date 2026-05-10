#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bank/bank_crypto.h"

int main(void)
{
    bank_set_log_level(BANK_LOG_INFO);
    bank_set_log_file("bank_crypto.log");

    bank_self_test();

    printf("\n");
    printf("========================================================================\n");
    printf("                   BANK CRYPTOGRAPHIC MODULE v3.0                       \n");
    printf("========================================================================\n");
    printf("Status: OPERATIONAL\n");
    printf("Algorithms: AES-256-GCM, ChaCha20-Poly1305, SHA-256, SHA-512, HMAC-SHA256\n");
    printf("           PBKDF2-HMAC-SHA256, HKDF, Poly1305, CRC32\n");
    printf("------------------------------------------------------------------------\n");
    printf("Configuration:\n");
    printf("  Max data size: %d bytes (%.2f MB)\n", BANK_MAX_DATA, (double)BANK_MAX_DATA / (1024 * 1024));
    printf("  Max sessions: %d\n", BANK_MAX_SESSION_KEYS);
    printf("  PBKDF2 iterations: %d\n", BANK_PBKDF2_ROUNDS);
    printf("  RNG pool size: %d bytes\n", BANK_RNG_POOL_SIZE);
    printf("  Max key rotations: %d\n", BANK_MAX_KEY_ROTATIONS);
    printf("------------------------------------------------------------------------\n");
    printf("Log file: bank_crypto.log\n");
    printf("========================================================================\n\n");

    printf("API Functions:\n");
    printf("  [INITIALIZATION]\n");
    printf("    bank_init_master_random() - Initialize with random master key\n");
    printf("    bank_init_master() - Initialize with provided master key\n");
    printf("    bank_rotate_master_key() - Rotate master key securely\n");
    printf("    bank_wipe_master() - Securely wipe all cryptographic material\n");
    printf("\n  [SESSION MANAGEMENT]\n");
    printf("    bank_create_session() - Create encryption session with TTL\n");
    printf("    bank_destroy_session() - Destroy session and wipe keys\n");
    printf("    bank_get_session_key() - Retrieve session keys\n");
    printf("\n  [ENCRYPTION/DECRYPTION]\n");
    printf("    bank_encrypt_data() - AES-256-GCM encryption\n");
    printf("    bank_decrypt_data() - AES-256-GCM decryption\n");
    printf("    bank_encrypt_chacha20_poly1305() - ChaCha20-Poly1305 encryption\n");
    printf("\n  [AUTHENTICATION]\n");
    printf("    bank_sign_data() / bank_verify_signature() - HMAC-SHA256\n");
    printf("    bank_hmac_sha256() - Direct HMAC computation\n");
    printf("\n  [TRANSACTIONS]\n");
    printf("    bank_create_transaction() - Create signed financial transaction\n");
    printf("    bank_verify_transaction() - Verify transaction signature\n");
    printf("\n  [SECURE ENVELOPES]\n");
    printf("    bank_create_envelope() - Create authenticated secure envelope\n");
    printf("    bank_extract_envelope() - Extract and verify envelope\n");
    printf("\n  [HASH & KEY DERIVATION]\n");
    printf("    bank_sha256() / bank_sha512() - Cryptographic hashing\n");
    printf("    bank_pbkdf2_hmac_sha256() - Password-based key derivation\n");
    printf("    bank_hkdf_extract() / bank_hkdf_expand() - HKDF key derivation\n");
    printf("    bank_derive_key_from_password() - High-level key derivation\n");
    printf("\n  [UTILITIES]\n");
    printf("    bank_random_bytes() - Cryptographically secure random\n");
    printf("    bank_crc32() / bank_crc32_combine() - CRC32 checksum\n");
    printf("    bank_secure_zero() - Secure memory zeroing\n");
    printf("    bank_secure_compare() - Timing-safe comparison\n");
    printf("    bank_get_stats() - Get library statistics\n");
    printf("========================================================================\n");

    uint64_t enc, dec, sess_created, sess_destroyed;
    int active;
    bank_get_stats(&enc, &dec, &sess_created, &sess_destroyed, &active);

    printf("\nRuntime Statistics:\n");
    printf("  Total encryptions: %llu\n", (unsigned long long)enc);
    printf("  Total decryptions: %llu\n", (unsigned long long)dec);
    printf("  Sessions created: %llu\n", (unsigned long long)sess_created);
    printf("  Sessions destroyed: %llu\n", (unsigned long long)sess_destroyed);
    printf("  Active sessions: %d\n", active);
    printf("  Library uptime: %ld seconds\n", (long)bank_get_library_uptime());
    printf("========================================================================\n\n");

    printf("Security Notes:\n");
    printf("  - All keys are zeroed after use\n");
    printf("  - Timing-safe comparison implemented\n");
    printf("  - Master key supports rotation\n");
    printf("  - Session keys have TTL expiration\n");
    printf("  - RNG with entropy pool\n");
    printf("  - Anti-tamper mechanisms active\n");
    printf("========================================================================\n");

    bank_close_log_file();

    return 0;
}
