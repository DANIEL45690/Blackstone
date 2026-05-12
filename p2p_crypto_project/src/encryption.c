#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>

int encrypt_data(CryptoContext *ctx, const byte *plaintext, size_t text_len, const byte *key, size_t key_len, const byte *iv, byte *ciphertext_out)
{
    size_t i;

    if (!ctx || !ctx->initialized || !plaintext || !key || !ciphertext_out)
        return CRYPTO_ERROR;
    (void)iv;

    lock_critical_section(ctx);

    setup_encryption_key(ctx, key, key_len);

    for (i = 0; i < text_len; i++)
    {
        byte val = plaintext[i];
        val ^= (byte)(ctx->key_schedule[i & 63] & 0xFF);
        val = ctx->sbox_byte[val];
        val = ctx->ternary_sbox[val];
        ciphertext_out[i] = val;
    }

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

int decrypt_data(CryptoContext *ctx, const byte *ciphertext, size_t text_len, const byte *key, size_t key_len, const byte *iv, byte *plaintext_out)
{
    size_t i;

    if (!ctx || !ctx->initialized || !ciphertext || !key || !plaintext_out)
        return CRYPTO_ERROR;
    (void)iv;

    lock_critical_section(ctx);

    setup_decryption_key(ctx, key, key_len);

    for (i = 0; i < text_len; i++)
    {
        byte val = ctx->ternary_inv_sbox[ciphertext[i]];
        val = ctx->inv_sbox[val];
        val ^= (byte)(ctx->key_schedule[i & 63] & 0xFF);
        plaintext_out[i] = val;
    }

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

void setup_encryption_key(CryptoContext *ctx, const byte *key, size_t key_len)
{
    size_t i;
    size_t copy_len = key_len < KEY_SCHEDULE_SIZE ? key_len : KEY_SCHEDULE_SIZE;

    memcpy(ctx->key_schedule, key, copy_len);

    for (i = 0; i < copy_len; i++)
    {
        ctx->key_schedule[i] = ctx->sbox_byte[ctx->key_schedule[i] & 0xFF];
    }
}

void setup_decryption_key(CryptoContext *ctx, const byte *key, size_t key_len)
{
    size_t i;
    size_t copy_len = key_len < KEY_SCHEDULE_SIZE ? key_len : KEY_SCHEDULE_SIZE;

    memcpy(ctx->key_schedule, key, copy_len);

    for (i = 0; i < copy_len; i++)
    {
        ctx->key_schedule[i] = ctx->inv_sbox[ctx->key_schedule[i] & 0xFF];
    }
}
