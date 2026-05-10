#include "bank/bank_hkdf.h"
#include "bank/bank_hmac.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include <string.h>

void bank_hkdf_extract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk)
{
    if (salt == NULL || salt_len == 0)
    {
        uint8_t zeros[BANK_HMAC_BLOCK_SIZE] = {0};
        bank_hmac_sha256(zeros, BANK_HMAC_BLOCK_SIZE, ikm, ikm_len, prk);
    }
    else
    {
        bank_hmac_sha256(salt, salt_len, ikm, ikm_len, prk);
    }
}

void bank_hkdf_expand(const uint8_t *prk, size_t prk_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len)
{
    (void)prk;
    (void)prk_len;

    uint8_t previous[BANK_SHA256_DIGEST_SIZE];
    uint8_t counter = 1;
    size_t produced = 0;

    while (produced < okm_len)
    {
        bank_sha256_ctx ctx;
        bank_sha256_init(&ctx);
        if (produced > 0)
        {
            bank_sha256_update(&ctx, previous, BANK_SHA256_DIGEST_SIZE);
        }
        bank_sha256_update(&ctx, info, info_len);
        bank_sha256_update(&ctx, &counter, 1);
        bank_sha256_final(&ctx, previous);

        size_t to_copy = okm_len - produced;
        if (to_copy > BANK_SHA256_DIGEST_SIZE)
            to_copy = BANK_SHA256_DIGEST_SIZE;
        memcpy(okm + produced, previous, to_copy);
        produced += to_copy;
        counter++;
    }
}

void bank_hkdf(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len,
               const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len)
{
    uint8_t prk[BANK_HKDF_EXTRACT_SIZE];
    bank_hkdf_extract(salt, salt_len, ikm, ikm_len, prk);
    bank_hkdf_expand(prk, BANK_HKDF_EXTRACT_SIZE, info, info_len, okm, okm_len);
    bank_secure_zero(prk, sizeof(prk));
}
