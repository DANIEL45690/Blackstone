#include "crypto_api.h"
#include "crypto_core.h"
#include <string.h>

void secure_memset(volatile void *ptr, int value, size_t num)
{
    volatile byte *p = (volatile byte *)ptr;
    while (num--)
        *p++ = (byte)value;
}

void secure_memcpy(void *dest, const void *src, size_t num)
{
    memcpy(dest, src, num);
}

int secure_compare(const void *a, const void *b, size_t num)
{
    const volatile byte *pa = (const volatile byte *)a;
    const volatile byte *pb = (const volatile byte *)b;
    volatile int result = 0;
    size_t i;

    for (i = 0; i < num; i++)
    {
        result |= (pa[i] ^ pb[i]);
    }

    return (result == 0) ? 1 : 0;
}

void wipe_memory(volatile void *ptr, size_t num)
{
    secure_memset(ptr, 0, num);
}

void wipe_all_secure_memory(CryptoContext *ctx)
{
    if (!ctx)
        return;

    wipe_memory(ctx->state, sizeof(ctx->state));
    wipe_memory(ctx->key_schedule, sizeof(ctx->key_schedule));
    wipe_memory(ctx->random_buffer, sizeof(ctx->random_buffer));
    wipe_memory(ctx->entropy_pool, sizeof(ctx->entropy_pool));
}

int protect_memory(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return CRYPTO_SUCCESS;
}

int unprotect_memory(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return CRYPTO_SUCCESS;
}
