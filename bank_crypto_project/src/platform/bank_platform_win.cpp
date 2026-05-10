#include "bank/bank_platform.h"

#ifdef BANK_PLATFORM_WINDOWS

#include <windows.h>
#include <stdio.h>

static CRITICAL_SECTION g_platform_mutex;

void bank_platform_init(void)
{
    InitializeCriticalSection(&g_platform_mutex);
}

void bank_platform_cleanup(void)
{
    DeleteCriticalSection(&g_platform_mutex);
}

int bank_platform_random_bytes(uint8_t *buf, size_t len)
{
    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return 0;
    if (!CryptGenRandom(hProv, (DWORD)len, buf))
    {
        CryptReleaseContext(hProv, 0);
        return 0;
    }
    CryptReleaseContext(hProv, 0);
    return 1;
}

uint64_t bank_platform_get_time(void)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    return time / 10000;
}

void bank_platform_sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

void bank_platform_lock_create(void **lock)
{
    CRITICAL_SECTION *cs = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs);
    *lock = cs;
}

void bank_platform_lock_destroy(void *lock)
{
    if (lock)
    {
        DeleteCriticalSection((CRITICAL_SECTION *)lock);
        free(lock);
    }
}

void bank_platform_lock_acquire(void *lock)
{
    EnterCriticalSection((CRITICAL_SECTION *)lock);
}

void bank_platform_lock_release(void *lock)
{
    LeaveCriticalSection((CRITICAL_SECTION *)lock);
}

#endif
