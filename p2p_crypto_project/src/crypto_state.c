#include "crypto_api.h"
#include <pthread.h>

static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void lock_critical_section(CryptoContext *ctx)
{
    (void)ctx;
    pthread_mutex_lock(&global_mutex);
}

void unlock_critical_section(CryptoContext *ctx)
{
    (void)ctx;
    pthread_mutex_unlock(&global_mutex);
}

u64 get_security_status(CryptoContext *ctx)
{
    if (!ctx)
        return 0;

    u64 status = ctx->security_flags | ctx->error_counter;
    status = (status << 32) | ctx->initialized;

    return status;
}
