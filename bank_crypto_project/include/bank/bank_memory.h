#ifndef BANK_MEMORY_H
#define BANK_MEMORY_H

#include "../bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void *bank_malloc_secure(size_t size);
    void bank_free_secure(void *ptr);
    void *bank_malloc_secure_aligned(size_t size, size_t alignment);
    void bank_protect_memory(void *ptr, size_t size);
    void bank_unprotect_memory(void *ptr, size_t size);
    void bank_lock_memory(void *ptr, size_t size);
    void bank_unlock_memory(void *ptr, size_t size);
    int bank_mprotect_readonly(void *ptr, size_t size);
    int bank_mprotect_readwrite(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif
