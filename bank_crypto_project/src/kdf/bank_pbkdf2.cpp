#include "bank/bank_pbkdf2.h"
#include "bank/bank_hmac.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include <string.h>

void bank_pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len,
                             const uint8_t *salt, size_t salt_len,
                             int iterations, uint8_t *key, size_t key_len)
{
    uint8_t digest[BANK_SHA256_DIGEST_SIZE];
    uint8_t salt_block[BANK_SHA256_BLOCK_SIZE + 4];
    uint8_t u[BANK_SHA256_DIGEST_SIZE];
    size_t key_idx = 0;
    int block = 1;

    while (key_idx < key_len)
    {
        memcpy(salt_block, salt, salt_len);
        salt_block[salt_len] = (block >> 24) & 0xFF;
        salt_block[salt_len + 1] = (block >> 16) & 0xFF;
        salt_block[salt_len + 2] = (block >> 8) & 0xFF;
        salt_block[salt_len + 3] = block & 0xFF;

        bank_hmac_sha256(password, password_len, salt_block, salt_len + 4, digest);
        memcpy(u, digest, BANK_SHA256_DIGEST_SIZE);

        for (int i = 1; i < iterations; i++)
        {
            bank_hmac_sha256(password, password_len, u, BANK_SHA256_DIGEST_SIZE, u);
            for (int j = 0; j < BANK_SHA256_DIGEST_SIZE; j++)
            {
                digest[j] ^= u[j];
            }
        }

        for (int i = 0; i < BANK_SHA256_DIGEST_SIZE && key_idx < key_len; i++)
        {
            key[key_idx++] = digest[i];
        }

        block++;
    }
}

int bank_derive_key_from_password(const uint8_t *password, size_t password_len,
                                  const uint8_t *salt, size_t salt_len,
                                  uint8_t *key, size_t key_len)
{
    if (!password || !salt || !key)
        return 0;

    bank_pbkdf2_hmac_sha256(password, password_len, salt, salt_len,
                            BANK_PBKDF2_ROUNDS, key, key_len);

    return 1;
}
