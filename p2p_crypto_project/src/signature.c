#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>

int compute_signature(CryptoContext *ctx, const byte *data, size_t data_len, const byte *key, size_t key_len, byte *sig_out)
{
    if (!ctx || !ctx->initialized || !data || !key || !sig_out)
        return CRYPTO_ERROR;

    lock_critical_section(ctx);

    compute_hash_secure(ctx, data, data_len, ctx->hash_buffer);
    random_bytes(ctx, (byte *)ctx->random_buffer, 32);

    memcpy(ctx->signature_buffer, ctx->hash_buffer, 64);
    apply_signature_algorithm(ctx, 64);

    memcpy(sig_out, ctx->signature_buffer, 128);

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

void apply_signature_algorithm(CryptoContext *ctx, size_t len)
{
    size_t i;
    for (i = 0; i < len && i < RANDOM_BUFFER_SIZE; i++)
    {
        byte val = ctx->signature_buffer[i];
        val ^= (byte)(ctx->random_buffer[i] & 0xFF);
        ctx->signature_buffer[i] = ctx->sbox_byte[val];
    }
}

int verify_signature(CryptoContext *ctx, const byte *data, size_t data_len, const byte *key, size_t key_len, const byte *signature)
{
    int result;

    if (!ctx || !ctx->initialized || !data || !key || !signature)
        return CRYPTO_ERROR;

    lock_critical_section(ctx);

    compute_hash_secure(ctx, data, data_len, ctx->hash_buffer);

    memcpy(ctx->binary_buffer, signature, 128);
    result = verify_signature_core(ctx, ctx->binary_buffer, 64);

    unlock_critical_section(ctx);

    return result;
}

int verify_signature_core(CryptoContext *ctx, const byte *signature, size_t sig_len)
{
    size_t i;

    for (i = 0; i < sig_len && i < HASH_SIZE; i++)
    {
        byte val = ctx->inv_sbox[signature[i]];
        byte key_byte = (byte)(ctx->random_buffer[i] & 0xFF);
        val ^= key_byte;
        if (val != ctx->hash_buffer[i])
            return CRYPTO_ERROR;
    }

    return CRYPTO_SUCCESS;
}
