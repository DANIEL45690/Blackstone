#include "crypto_api.h"
#include "crypto_core.h"

static u8 binary_encode_byte(u8 b)
{
    u8 result = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        result <<= 1;
        result |= (b >> 7) & 1;
        b <<= 1;
    }
    return result;
}

static u8 binary_decode_byte(u8 b)
{
    u8 result = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        result <<= 1;
        result |= (b >> 7) & 1;
        b <<= 1;
    }
    return result;
}

size_t binary_encode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output)
{
    size_t i;
    if (!ctx || !ctx->initialized || !input || !output)
        return 0;

    for (i = 0; i < input_len; i++)
    {
        output[i] = binary_encode_byte(input[i]);
    }

    return input_len;
}

size_t binary_decode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output)
{
    size_t i;
    if (!ctx || !ctx->initialized || !input || !output)
        return 0;

    for (i = 0; i < input_len; i++)
    {
        output[i] = binary_decode_byte(input[i]);
    }

    return input_len;
}

size_t ternary_encode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output)
{
    size_t i;
    if (!ctx || !ctx->initialized || !input || !output)
        return 0;

    for (i = 0; i < input_len; i++)
    {
        output[i] = ctx->ternary_sbox[input[i]];
    }

    return input_len;
}

size_t ternary_decode(CryptoContext *ctx, const byte *input, size_t input_len, byte *output)
{
    size_t i;
    if (!ctx || !ctx->initialized || !input || !output)
        return 0;

    for (i = 0; i < input_len; i++)
    {
        output[i] = ctx->ternary_inv_sbox[input[i]];
    }

    return input_len;
}
