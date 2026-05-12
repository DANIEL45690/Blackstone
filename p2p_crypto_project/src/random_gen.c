#include "crypto_api.h"
#include "crypto_core.h"

int random_bytes(CryptoContext *ctx, byte *buffer, size_t buffer_len)
{
    size_t i;
    u64 rnd;

    if (!ctx || !buffer)
        return CRYPTO_ERROR;

    for (i = 0; i < buffer_len; i++)
    {
        rnd = rdrand_wrapper();
        buffer[i] = (byte)(rnd & 0xFF);
    }

    return CRYPTO_SUCCESS;
}
