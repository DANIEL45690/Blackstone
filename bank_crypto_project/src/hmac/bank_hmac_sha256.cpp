#include "bank/bank_hmac.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include <string.h>

void bank_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac)
{
    uint8_t ipad[BANK_HMAC_BLOCK_SIZE];
    uint8_t opad[BANK_HMAC_BLOCK_SIZE];
    uint8_t inner_hash[BANK_SHA256_DIGEST_SIZE];
    bank_sha256_ctx ctx;

    memset(ipad, 0, BANK_HMAC_BLOCK_SIZE);
    memset(opad, 0, BANK_HMAC_BLOCK_SIZE);

    if (key_len > BANK_HMAC_BLOCK_SIZE)
    {
        bank_sha256(key, key_len, ipad);
        memcpy(opad, ipad, BANK_SHA256_DIGEST_SIZE);
    }
    else
    {
        memcpy(ipad, key, key_len);
        memcpy(opad, key, key_len);
    }

    for (size_t i = 0; i < BANK_HMAC_BLOCK_SIZE; i++)
    {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }

    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, ipad, BANK_HMAC_BLOCK_SIZE);
    bank_sha256_update(&ctx, data, data_len);
    bank_sha256_final(&ctx, inner_hash);

    bank_sha256_init(&ctx);
    bank_sha256_update(&ctx, opad, BANK_HMAC_BLOCK_SIZE);
    bank_sha256_update(&ctx, inner_hash, BANK_SHA256_DIGEST_SIZE);
    bank_sha256_final(&ctx, mac);
}
