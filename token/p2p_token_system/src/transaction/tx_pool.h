#ifndef P2P_TX_POOL_H
#define P2P_TX_POOL_H

#include "../common/types.h"

typedef struct tx_pool {
    transaction_t* transactions;
    uint64_t head;
    uint64_t tail;
    volatile uint64_t size;
    uint64_t capacity;
    CRITICAL_SECTION lock;
    hash_t* hashes;
} tx_pool_t;

tx_pool_t* tx_pool_create(uint64_t capacity);
void tx_pool_destroy(tx_pool_t* pool);
bool tx_pool_add(tx_pool_t* pool, const transaction_t* tx);
transaction_t* tx_pool_pop(tx_pool_t* pool);
transaction_t* tx_pool_peek(tx_pool_t* pool);
bool tx_pool_contains(tx_pool_t* pool, const hash_t* tx_hash);
void tx_pool_cleanup(tx_pool_t* pool);
void tx_compute_hash(transaction_t* tx);
bool tx_verify(const transaction_t* tx, void* balance_cache, uint32_t current_height);

#endif