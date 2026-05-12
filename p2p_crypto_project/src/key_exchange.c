#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>

int generate_random_key(CryptoContext *ctx, byte *key_out, size_t key_len)
{
    if (!ctx || !ctx->initialized || !key_out)
        return CRYPTO_ERROR;

    lock_critical_section(ctx);

    random_bytes(ctx, key_out, key_len);
    apply_key_derivation(ctx, key_out, key_len);

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

void apply_key_derivation(CryptoContext *ctx, byte *key, size_t key_len)
{
    size_t i;
    for (i = 0; i < key_len; i++)
    {
        byte val = ctx->sbox_byte[key[i]];
        val = ctx->ternary_sbox[val];
        key[i] ^= val;
    }
}

int key_exchange(CryptoContext *ctx, const byte *public_key, size_t key_len, byte *shared_secret_out, byte *session_key_out)
{
    if (!ctx || !ctx->initialized || !public_key || !shared_secret_out || !session_key_out)
        return CRYPTO_ERROR;

    lock_critical_section(ctx);

    random_bytes(ctx, (byte *)ctx->random_buffer, 32);
    compute_exchange_phase1(ctx, public_key, key_len);

    memcpy(shared_secret_out, ctx->key_schedule, 32);

    compute_exchange_phase2(ctx, (byte *)ctx->random_buffer, 32);

    memcpy(session_key_out, ctx->key_schedule, 32);

    unlock_critical_section(ctx);

    return CRYPTO_SUCCESS;
}

void compute_exchange_phase1(CryptoContext *ctx, const byte *input, size_t len)
{
    size_t i;
    size_t copy_len = len < KEY_SCHEDULE_SIZE ? len : KEY_SCHEDULE_SIZE;

    for (i = 0; i < copy_len; i++)
    {
        byte val = ctx->sbox_byte[input[i]];
        val = ctx->ternary_sbox[val];
        ctx->key_schedule[i] = input[i] ^ val;
    }
}

void compute_exchange_phase2(CryptoContext *ctx, const byte *input, size_t len)
{
    size_t i;
    size_t copy_len = len < KEY_SCHEDULE_SIZE ? len : KEY_SCHEDULE_SIZE;

    for (i = 0; i < copy_len; i++)
    {
        byte val = input[i] ^ (ctx->key_schedule[i] & 0xFF);
        ctx->key_schedule[i] = ctx->inv_sbox[val];
    }
}
