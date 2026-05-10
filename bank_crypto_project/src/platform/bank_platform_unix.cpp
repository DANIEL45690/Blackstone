#include "bank/bank_platform.h"

#if defined(BANK_PLATFORM_LINUX) || defined(BANK_PLATFORM_MACOS)

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

static pthread_mutex_t g_platform_mutex = PTHREAD_MUTEX_INITIALIZER;

void bank_platform_init(void)
{
    pthread_mutex_init(&g_platform_mutex, NULL);
}

void bank_platform_cleanup(void)
{
    pthread_mutex_destroy(&g_platform_mutex);
}

int bank_platform_random_bytes(uint8_t *buf, size_t len)
{
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f)
        return 0;
    size_t rd = fread(buf, 1, len, f);
    fclose(f);
    return (rd == len) ? 1 : 0;
}

uint64_t bank_platform_get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

void bank_platform_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

void bank_platform_lock_create(void **lock)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    *lock = mutex;
}

void bank_platform_lock_destroy(void *lock)
{
    if (lock)
    {
        pthread_mutex_destroy((pthread_mutex_t *)lock);
        free(lock);
    }
}

void bank_platform_lock_acquire(void *lock)
{
    pthread_mutex_lock((pthread_mutex_t *)lock);
}

void bank_platform_lock_release(void *lock)
{
    pthread_mutex_unlock((pthread_mutex_t *)lock);
}

#endif
