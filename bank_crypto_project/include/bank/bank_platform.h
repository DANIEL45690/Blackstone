#ifndef BANK_PLATFORM_H
#define BANK_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
#define BANK_PLATFORM_WINDOWS 1
#include <windows.h>
#include <wincrypt.h>
#include <synchapi.h>
#elif defined(__linux__)
#define BANK_PLATFORM_LINUX 1
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#elif defined(__APPLE__)
#define BANK_PLATFORM_MACOS 1
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define BANK_ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define BANK_ARCH_ARM64 1
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_platform_init(void);
    void bank_platform_cleanup(void);
    int bank_platform_random_bytes(uint8_t *buf, size_t len);
    uint64_t bank_platform_get_time(void);
    void bank_platform_sleep_ms(uint32_t ms);
    void bank_platform_lock_create(void **lock);
    void bank_platform_lock_destroy(void *lock);
    void bank_platform_lock_acquire(void *lock);
    void bank_platform_lock_release(void *lock);

#ifdef __cplusplus
}
#endif

#endif
