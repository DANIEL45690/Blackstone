#ifndef P2P_BALANCE_CACHE_H
#define P2P_BALANCE_CACHE_H

#include "../common/types.h"

typedef struct balance_entry {
    address_t address;
    token_id_t token_id;
    amount_t balance;
    uint32_t last_update;
    amount_t locked;
} balance_entry_t;

typedef struct balance_cache {
    balance_entry_t* entries;
    size_t capacity;
    size_t count;
    CRITICAL_SECTION lock;
    uint64_t hits;
    uint64_t misses;
} balance_cache_t;

balance_cache_t* balance_cache_create(size_t capacity);
void balance_cache_destroy(balance_cache_t* cache);
void balance_cache_update(balance_cache_t* cache, const address_t* addr, token_id_t token_id, amount_t delta);
amount_t balance_cache_get(balance_cache_t* cache, const address_t* addr, token_id_t token_id);
double balance_cache_get_hit_rate(balance_cache_t* cache);

#endif