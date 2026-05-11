#include "tx_pool.h"
#include "../crypto/hash.h"

tx_pool_t* tx_pool_create(uint64_t capacity) {
    tx_pool_t* pool = (tx_pool_t*)calloc(1, sizeof(tx_pool_t));
    if (!pool) return NULL;

    pool->transactions = (transaction_t*)calloc(capacity, sizeof(transaction_t));
    pool->hashes = (hash_t*)calloc(capacity, sizeof(hash_t));

    if (!pool->transactions || !pool->hashes) {
        free(pool->transactions);
        free(pool->hashes);
        free(pool);
        return NULL;
    }

    pool->capacity = capacity;
    pool->head = 0;
    pool->tail = 0;
    pool->size = 0;
    InitializeCriticalSection(&pool->lock);

    return pool;
}

void tx_pool_destroy(tx_pool_t* pool) {
    if (!pool) return;

    DeleteCriticalSection(&pool->lock);
    free(pool->transactions);
    free(pool->hashes);
    free(pool);
}

bool tx_pool_add(tx_pool_t* pool, const transaction_t* tx) {
    if (!pool || !tx) return false;

    EnterCriticalSection(&pool->lock);

    if (pool->size >= pool->capacity) {
        LeaveCriticalSection(&pool->lock);
        return false;
    }

    uint64_t idx = pool->tail % pool->capacity;
    memcpy(&pool->transactions[idx], tx, sizeof(transaction_t));
    pool->hashes[idx] = tx->tx_hash;
    pool->tail++;
    pool->size++;

    LeaveCriticalSection(&pool->lock);
    return true;
}

transaction_t* tx_pool_pop(tx_pool_t* pool) {
    if (!pool) return NULL;

    EnterCriticalSection(&pool->lock);

    if (pool->size == 0) {
        LeaveCriticalSection(&pool->lock);
        return NULL;
    }

    uint64_t idx = pool->head % pool->capacity;
    transaction_t* tx = (transaction_t*)malloc(sizeof(transaction_t));
    if (tx) {
        memcpy(tx, &pool->transactions[idx], sizeof(transaction_t));
    }
    pool->head++;
    pool->size--;

    LeaveCriticalSection(&pool->lock);
    return tx;
}

transaction_t* tx_pool_peek(tx_pool_t* pool) {
    if (!pool) return NULL;

    EnterCriticalSection(&pool->lock);

    if (pool->size == 0) {
        LeaveCriticalSection(&pool->lock);
        return NULL;
    }

    uint64_t idx = pool->head % pool->capacity;
    transaction_t* tx = (transaction_t*)malloc(sizeof(transaction_t));
    if (tx) {
        memcpy(tx, &pool->transactions[idx], sizeof(transaction_t));
    }

    LeaveCriticalSection(&pool->lock);
    return tx;
}

bool tx_pool_contains(tx_pool_t* pool, const hash_t* tx_hash) {
    if (!pool || !tx_hash) return false;

    EnterCriticalSection(&pool->lock);

    uint64_t current = pool->head;
    bool found = false;

    while (current < pool->tail) {
        uint64_t idx = current % pool->capacity;
        if (memcmp(&pool->hashes[idx], tx_hash, HASH_LEN) == 0) {
            found = true;
            break;
        }
        current++;
    }

    LeaveCriticalSection(&pool->lock);
    return found;
}

void tx_pool_cleanup(tx_pool_t* pool) {
    if (!pool) return;

    EnterCriticalSection(&pool->lock);

    timestamp_t now = time(NULL);
    uint64_t current = pool->head;

    while (current < pool->tail) {
        uint64_t idx = current % pool->capacity;
        if (now - pool->transactions[idx].timestamp > 3600) {
            pool->head++;
            pool->size--;
        }
        current++;
    }

    LeaveCriticalSection(&pool->lock);
}

void tx_compute_hash(transaction_t* tx) {
    if (!tx) return;

    uint8_t buffer[256];
    size_t offset = 0;

    memcpy(buffer + offset, &tx->from, ADDRESS_LEN); offset += ADDRESS_LEN;
    memcpy(buffer + offset, &tx->to, ADDRESS_LEN); offset += ADDRESS_LEN;
    memcpy(buffer + offset, &tx->amount, sizeof(amount_t)); offset += sizeof(amount_t);
    memcpy(buffer + offset, &tx->token_id, sizeof(token_id_t)); offset += sizeof(token_id_t);
    memcpy(buffer + offset, &tx->timestamp, sizeof(timestamp_t)); offset += sizeof(timestamp_t);
    memcpy(buffer + offset, &tx->nonce, sizeof(nonce_t)); offset += sizeof(nonce_t);
    memcpy(buffer + offset, &tx->fee, sizeof(uint32_t)); offset += sizeof(uint32_t);

    hash_compute(buffer, offset, &tx->tx_hash);
}

bool tx_verify(const transaction_t* tx, void* cache, uint32_t current_height) {
    (void)cache;
    (void)current_height;

    if (!tx) return false;
    if (tx->amount == 0) return false;
    if (tx->fee > tx->amount / 10) return false;
    if (memcmp(&tx->from, &tx->to, ADDRESS_LEN) == 0) return false;

    return true;
}