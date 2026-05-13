#include "test_crypto.h"
#include "../core/logger.h"
#include "../core/crypto.h"

int run_crypto_tests(void)
{
    LOG_INFO("========== CRYPTO TESTS ==========");

    if (!crypto_init())
    {
        LOG_ERROR("Crypto init failed");
        return 0;
    }
    LOG_INFO("[OK] Crypto provider initialized");

    uint8_t test_key[32];
    uint8_t test_data[128];
    uint8_t test_hash[SHA256_DIGEST_SIZE];

    random_bytes(test_key, 32);
    for (size_t i = 0; i < sizeof(test_data); i++)
    {
        test_data[i] = (uint8_t)i;
    }

    sha256(test_data, sizeof(test_data), test_hash);
    LOG_INFO("[OK] SHA-256 hash computed");

    uint32_t test_crc = crc32(test_data, sizeof(test_data));
    LOG_INFO("[OK] CRC32: 0x%08X", test_crc);

    uint8_t encrypted[128];
    memcpy(encrypted, test_data, sizeof(test_data));
    simple_xor_encrypt(encrypted, sizeof(encrypted), test_key, 32);

    uint8_t decrypted[128];
    memcpy(decrypted, encrypted, sizeof(encrypted));
    simple_xor_decrypt(decrypted, sizeof(decrypted), test_key, 32);

    if (memcmp(test_data, decrypted, sizeof(test_data)) == 0)
    {
        LOG_INFO("[OK] XOR encryption/decryption");
    }
    else
    {
        LOG_ERROR("[FAIL] XOR encryption");
    }

    crypto_cleanup();
    LOG_INFO("=================================");
    return 1;
}
