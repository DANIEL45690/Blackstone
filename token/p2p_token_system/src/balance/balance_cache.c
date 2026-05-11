#include "balance_cache.h"

static uint32_t hash_address(const address_t* addr, token_id_t token_id) {
    uint32_t h = 0;
    for (int i = 0; i < ADDRESS_LEN; i++) {
        h = h * 31 + addr->bytes[i];
    }
    h = h * 31 + token_id;
    return h;
}

balance_cache_t* balance_cache_create(size_t capacity) {
    balance_cache_t* cache = (balance_cache_t*)calloc(1, sizeof(balance_cache_t));
    if (!cache) return NULL;

    cache->entries = (balance_entry_t*)calloc(capacity, sizeof(balance_entry_t));
    if (!cache->entries) {
        free(cache);
        return NULL;
    }

    cache->capacity = capacity;
    cache->count = 0;
    cache->hits = 0;
    cache->misses = 0;
    InitializeCriticalSection(&cache->lock);

    return cache;
}

void balance_cache_destroy(balance_cache_t* cache) {
    if (!cache) return;

    DeleteCriticalSection(&cache->lock);
    free(cache->entries);
    free(cache);
}

void balance_cache_update(balance_cache_t* cache, const address_t* addr, token_id_t token_id, amount_t delta) {
    if (!cache || !addr) return;

    EnterCriticalSection(&cache->lock);

    uint32_t idx = hash_address(addr, token_id) % cache->capacity;
    uint32_t start = idx;

    do {
        if (cache->entries[idx].last_update == 0) {
            cache->entries[idx].address = *addr;
            cache->entries[idx].token_id = token_id;
            cache->entries[idx].balance = delta;
            cache->entries[idx].last_update = 1;
            cache->entries[idx].locked = 0;
            cache->count++;
            break;
        }

        if (memcmp(&cache->entries[idx].address, addr, ADDRESS_LEN) == 0 &&
            cache->entries[idx].token_id == token_id) {
            cache->entries[idx].balance += delta;
            break;
        }

        idx = (idx + 1) % cache->capacity;
    } while (idx != start);

    LeaveCriticalSection(&cache->lock);
}

amount_t balance_cache_get(balance_cache_t* cache, const address_t* addr, token_id_t token_id) {
    if (!cache || !addr) return 0;

    EnterCriticalSection(&cache->lock);

    uint32_t idx = hash_address(addr, token_id) % cache->capacity;
    uint32_t start = idx;
    amount_t result = 0;
    bool found = false;

    do {
        if (memcmp(&cache->entries[idx].address, addr, ADDRESS_LEN) == 0 &&
            cache->entries[idx].token_id == token_id) {
            result = cache->entries[idx].balance;
            found = true;
            break;
        }
        idx = (idx + 1) % cache->capacity;
    } while (idx != start && cache->entries[idx].last_update != 0);

    if (found) {
        cache->hits++;
    } else {
        cache->misses++;
    }

    LeaveCriticalSection(&cache->lock);
    return result;
}

double balance_cache_get_hit_rate(balance_cache_t* cache) {
    if (!cache) return 0.0;

    uint64_t total = cache->hits + cache->misses;
    if (total == 0) return 0.0;

    return (double)cache->hits / total * 100.0;
}