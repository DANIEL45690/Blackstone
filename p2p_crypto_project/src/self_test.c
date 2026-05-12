#include "crypto_api.h"
#include <string.h>

static const byte test_vector[] = "P2P Token System Cryptographic Test Vector 2024";
static const byte expected_hash[64] = {0x5A};
static const byte test_key[32] = {0x1F};
static const byte test_data[64] = {0x3C};
static const u64 test_iv = 0x1234567890ABCDEFULL;

int crypto_self_test(CryptoContext *ctx)
{
    byte hash_buffer[64];
    byte encrypted[64];
    byte decrypted[64];
    byte key[32];
    int result;

    if (!ctx || !ctx->initialized)
        return CRYPTO_ERROR;

    memcpy(key, test_key, 32);

    result = compute_hash_secure(ctx, test_vector, sizeof(test_vector) - 1, hash_buffer);
    if (result != CRYPTO_SUCCESS)
        goto fail;

    result = encrypt_data(ctx, test_data, 64, key, 32, (byte *)&test_iv, encrypted);
    if (result != CRYPTO_SUCCESS)
        goto fail;

    result = decrypt_data(ctx, encrypted, 64, key, 32, (byte *)&test_iv, decrypted);
    if (result != CRYPTO_SUCCESS)
        goto fail;

    result = secure_compare(test_data, decrypted, 64);
    if (result != 1)
        goto fail;

    return CRYPTO_SUCCESS;

fail:
    if (ctx)
        ctx->error_counter++;
    return CRYPTO_ERROR;
}
