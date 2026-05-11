#ifndef P2P_TOKEN_SYSTEM_H
#define P2P_TOKEN_SYSTEM_H

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <process.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

#define TOKEN_SYMBOL_LEN 8
#define TOKEN_NAME_LEN 32
#define ADDRESS_LEN 64
#define SIGNATURE_LEN 128
#define HASH_LEN 64
#define TX_ID_LEN 64
#define MAX_TOKENS 50000
#define MAX_TRANSACTIONS 5000000
#define MAX_PEERS 1024
#define MAX_BLOCKS 500000
#define BLOCK_SIZE 8192
#define TRANSACTION_POOL_SIZE 250000
#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define MAX_CONTRACT_SIZE 65536
#define MAX_ORDER_BOOK_DEPTH 10000
#define MAX_CONSENSUS_NODES 255
#define MAX_SMART_CONTRACTS 10000
#define MAX_CROSS_CHAIN_PEERS 512
#define MAX_GOVERNANCE_PROPOSALS 5000
#define MAX_STAKING_VALIDATORS 1000
#define MAX_LIQUIDITY_POOLS 500
#define MAX_NFT_COLLECTIONS 1000
#define MAX_NFT_ITEMS 100000
#define MAX_ATOMIC_SWAPS 10000

typedef uint64_t timestamp_t;
typedef uint64_t amount_t;
typedef uint64_t nonce_t;
typedef uint32_t token_id_t;
typedef uint32_t peer_id_t;
typedef uint32_t block_height_t;
typedef uint32_t contract_id_t;
typedef uint32_t validator_id_t;
typedef uint32_t proposal_id_t;
typedef uint32_t pool_id_t;
typedef uint32_t collection_id_t;
typedef uint64_t nft_id_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    uint8_t bytes[HASH_LEN];
} hash_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    uint8_t bytes[ADDRESS_LEN];
} address_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    char symbol[TOKEN_SYMBOL_LEN];
    char name[TOKEN_NAME_LEN];
    token_id_t id;
    address_t issuer;
    amount_t total_supply;
    amount_t circulating_supply;
    uint8_t decimals;
    timestamp_t created_at;
    uint32_t holders_count;
    bool is_frozen;
    bool is_mintable;
    bool is_burnable;
    uint32_t version;
    hash_t metadata_hash;
    amount_t max_supply;
    uint32_t transfer_fee_bps;
    address_t fee_collector;
} token_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    address_t from;
    address_t to;
    amount_t amount;
    token_id_t token_id;
    timestamp_t timestamp;
    nonce_t nonce;
    hash_t tx_hash;
    hash_t signature;
    uint32_t fee;
    uint8_t version;
    bool is_processed;
    uint32_t gas_price;
    uint64_t gas_limit;
    contract_id_t contract_id;
    uint8_t tx_type;
    uint32_t deadline;
    hash_t memo;
} transaction_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    hash_t block_hash;
    hash_t prev_block_hash;
    timestamp_t timestamp;
    block_height_t height;
    uint32_t tx_count;
    uint32_t nonce;
    hash_t merkle_root;
    address_t miner;
    amount_t reward;
    uint32_t difficulty;
    uint64_t total_fees;
    hash_t state_root;
    hash_t receipts_root;
    transaction_t *transactions[BLOCK_SIZE];
    uint32_t version;
    uint32_t gas_used;
    uint32_t gas_limit;
} block_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    address_t address;
    token_id_t token_id;
    amount_t balance;
    uint32_t last_update_block;
    uint32_t last_transaction_nonce;
    amount_t locked_balance;
    uint32_t lock_until_block;
} balance_entry_t;

typedef struct
{
    balance_entry_t *entries;
    size_t capacity;
    size_t count;
    CRITICAL_SECTION lock;
    uint32_t *index_map;
    volatile uint64_t cache_hits;
    volatile uint64_t cache_misses;
} balance_cache_t;

typedef struct
{
    transaction_t *pool;
    volatile uint64_t head;
    volatile uint64_t tail;
    volatile uint64_t size;
    CRITICAL_SECTION lock;
    uint64_t capacity;
    volatile uint64_t total_added;
    volatile uint64_t total_removed;
    HANDLE *wait_events;
} transaction_pool_t;

typedef struct
{
    block_t **blocks;
    volatile block_height_t height;
    CRITICAL_SECTION chain_lock;
    hash_t *block_index;
    uint32_t *difficulty_target;
    uint64_t *block_timestamps;
    volatile uint64_t total_difficulty;
    hash_t *block_hashes_by_height;
    CRITICAL_SECTION prune_lock;
    uint32_t prune_height;
} blockchain_t;

typedef struct
{
    token_t *tokens;
    uint32_t token_count;
    CRITICAL_SECTION registry_lock;
    hash_t *token_hashes;
    uint32_t *token_id_map;
    uint32_t *token_symbol_map;
    amount_t *token_volumes;
    uint32_t *token_holders_count;
} token_registry_t;

typedef struct
{
    peer_id_t id;
    address_t address;
    char ip[16];
    uint16_t port;
    uint32_t latency_ms;
    timestamp_t last_seen;
    uint32_t version;
    uint32_t score;
    bool is_active;
    uint32_t connected_peers;
    amount_t bandwidth_used;
    uint32_t blocks_sent;
    uint32_t blocks_received;
    CRITICAL_SECTION peer_lock;
    uint32_t fail_count;
    uint32_t reputation;
} peer_t;

typedef struct
{
    peer_t peers[MAX_PEERS];
    uint32_t peer_count;
    CRITICAL_SECTION peer_lock;
    peer_id_t *connection_ranks;
    uint32_t *peer_bandwidth;
    HANDLE *peer_events;
    volatile uint32_t active_connections;
} peer_manager_t;

typedef struct
{
    transaction_pool_t *tx_pool;
    blockchain_t *blockchain;
    token_registry_t *token_registry;
    balance_cache_t *balance_cache;
    peer_manager_t *peer_manager;
    volatile bool running;
    HANDLE worker_threads[64];
    HANDLE network_thread;
    HANDLE consensus_thread;
    HANDLE cleanup_thread;
    HANDLE metrics_thread;
    HANDLE validation_thread;
    uint32_t worker_count;
    volatile uint64_t total_transactions_processed;
    volatile uint64_t total_blocks_mined;
    volatile uint64_t total_peers_connected;
    CRITICAL_SECTION metrics_lock;
    timestamp_t start_time;
    uint32_t *thread_affinities;
    uint32_t target_block_time_ms;
    uint32_t current_total_peers;
} p2p_engine_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    hash_t tx_hash;
    hash_t block_hash;
    timestamp_t timestamp;
    uint32_t confirmations;
    uint8_t status;
    uint32_t gas_used;
    amount_t fee_paid;
    uint32_t block_height;
} transaction_status_t;

typedef struct
{
    uint64_t *rings;
    uint64_t ring_size;
    volatile uint64_t head;
    volatile uint64_t tail;
    volatile uint64_t count;
    HANDLE semaphore;
} lockfree_queue_t;

typedef struct
{
    uint8_t *memory_pool;
    size_t pool_size;
    volatile uint64_t offset;
    CRITICAL_SECTION alloc_lock;
    uint64_t *free_list;
    uint64_t free_count;
    uint64_t allocation_count;
    uint64_t deallocation_count;
} slab_allocator_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    address_t from;
    address_t to;
    token_id_t token_in;
    token_id_t token_out;
    amount_t amount_in;
    amount_t min_amount_out;
    timestamp_t deadline;
    hash_t swap_hash;
    uint32_t fee_bps;
    bool is_active;
} swap_order_t;

typedef struct
{
    swap_order_t *orders;
    uint32_t order_count;
    uint32_t max_orders;
    CRITICAL_SECTION swap_lock;
    uint64_t *price_levels;
    uint64_t total_volume_24h;
} dex_order_book_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    validator_id_t id;
    address_t address;
    amount_t stake;
    amount_t rewards;
    uint32_t blocks_validated;
    uint32_t missed_blocks;
    uint32_t uptime_percent;
    timestamp_t registered_at;
    bool is_active;
    uint32_t commission_bps;
    address_t reward_address;
    uint32_t version;
} validator_t;

typedef struct
{
    validator_t validators[MAX_STAKING_VALIDATORS];
    uint32_t validator_count;
    CRITICAL_SECTION staking_lock;
    amount_t total_staked;
    uint32_t *validator_ranks;
    amount_t *delegation_pools;
    uint32_t min_stake_amount;
    uint32_t epoch_duration;
    uint32_t current_epoch;
} staking_manager_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    proposal_id_t id;
    address_t proposer;
    char title[256];
    char description[4096];
    uint32_t start_epoch;
    uint32_t end_epoch;
    amount_t for_votes;
    amount_t against_votes;
    amount_t abstain_votes;
    uint8_t status;
    uint32_t execution_height;
    uint8_t proposal_type;
    uint8_t *execution_payload;
    uint32_t payload_size;
} governance_proposal_t;

typedef struct
{
    governance_proposal_t proposals[MAX_GOVERNANCE_PROPOSALS];
    uint32_t proposal_count;
    CRITICAL_SECTION governance_lock;
    amount_t min_voting_power;
    uint32_t voting_delay_blocks;
    uint32_t voting_period_blocks;
    address_t governance_token;
} governance_manager_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    pool_id_t id;
    address_t token_a;
    address_t token_b;
    amount_t reserve_a;
    amount_t reserve_b;
    amount_t total_liquidity;
    uint32_t fee_bps;
    timestamp_t created_at;
    amount_t volume_24h;
    uint32_t lp_token_id;
    address_t pool_address;
    bool is_active;
    amount_t reserve_a_cumulative;
    amount_t reserve_b_cumulative;
} liquidity_pool_t;

typedef struct
{
    liquidity_pool_t pools[MAX_LIQUIDITY_POOLS];
    uint32_t pool_count;
    CRITICAL_SECTION pool_lock;
    uint64_t *pool_slippages;
    amount_t *pool_apr;
} liquidity_manager_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    collection_id_t id;
    char name[64];
    char symbol[16];
    address_t owner;
    uint32_t total_supply;
    uint32_t max_supply;
    timestamp_t created_at;
    bool is_frozen;
    hash_t metadata_uri;
    amount_t royalty_bps;
    address_t royalty_recipient;
} nft_collection_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    nft_id_t id;
    collection_id_t collection_id;
    address_t owner;
    uint32_t token_id;
    hash_t metadata_hash;
    timestamp_t minted_at;
    amount_t last_sale_price;
    uint32_t transfer_count;
    bool is_listed;
    amount_t listed_price;
    address_t creator;
    uint32_t royalty_paid;
} nft_item_t;

typedef struct
{
    nft_collection_t collections[MAX_NFT_COLLECTIONS];
    nft_item_t items[MAX_NFT_ITEMS];
    uint32_t collection_count;
    uint32_t item_count;
    CRITICAL_SECTION nft_lock;
    uint32_t *collection_index_map;
    uint64_t *nft_owner_index;
} nft_manager_t;

typedef struct __attribute__((aligned(CACHE_LINE_SIZE)))
{
    hash_t swap_id;
    address_t initiator;
    address_t counterparty;
    address_t asset_a;
    address_t asset_b;
    amount_t amount_a;
    amount_t amount_b;
    hash_t secret_hash;
    uint8_t secret[32];
    timestamp_t timeout;
    uint8_t status;
    uint8_t version;
} atomic_swap_t;

typedef struct
{
    atomic_swap_t swaps[MAX_ATOMIC_SWAPS];
    uint32_t swap_count;
    CRITICAL_SECTION swap_lock;
    hash_t *swap_index;
    HANDLE *swap_timers;
} atomic_swap_manager_t;

static inline uint64_t rdtsc(void)
{
    return __rdtsc();
}

static inline void atomic_increment(volatile uint64_t *ptr)
{
    InterlockedIncrement64((volatile LONG64 *)ptr);
}

static inline void atomic_decrement(volatile uint64_t *ptr)
{
    InterlockedDecrement64((volatile LONG64 *)ptr);
}

static inline uint64_t atomic_add(volatile uint64_t *ptr, uint64_t value)
{
    return InterlockedExchangeAdd64((volatile LONG64 *)ptr, value);
}

static inline bool atomic_cas(volatile uint64_t *ptr, uint64_t old_val, uint64_t new_val)
{
    return InterlockedCompareExchange64((volatile LONG64 *)ptr, new_val, old_val) == old_val;
}

static inline void prefetch(const void *addr)
{
    __faststorefence();
}

static inline void mfence(void)
{
    MemoryBarrier();
}

static void *aligned_alloc_custom(size_t alignment, size_t size)
{
    return _aligned_malloc(size, alignment);
}

static void aligned_free_custom(void *ptr)
{
    _aligned_free(ptr);
}

static hash_t compute_hash(const uint8_t *data, size_t len)
{
    hash_t result = {0};
    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; i++)
    {
        h1 ^= data[i];
        h1 *= 0x100000001b3ULL;
        h2 ^= data[i] + 0x9e3779b97f4a7c15ULL;
        h2 *= 0xbf58476d1ce4e5b9ULL;
    }
    memcpy(result.bytes, &h1, 8);
    memcpy(result.bytes + 8, &h2, 8);
    for (int i = 16; i < 64; i++)
    {
        result.bytes[i] = result.bytes[i - 16] ^ result.bytes[i - 8];
    }
    return result;
}

static hash_t compute_hash_fast(const uint8_t *data, size_t len)
{
    hash_t result = {0};
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++)
    {
        h ^= data[i];
        h *= 0x100000001b3ULL;
    }
    memcpy(result.bytes, &h, 8);
    for (int i = 8; i < 64; i++)
    {
        result.bytes[i] = result.bytes[i - 8] ^ (i * 0xFF);
    }
    return result;
}

static hash_t compute_merkle_root(transaction_t **txs, uint32_t count)
{
    if (count == 0)
    {
        hash_t empty = {0};
        return empty;
    }
    if (count == 1)
    {
        return compute_hash((uint8_t *)&txs[0]->tx_hash, sizeof(hash_t));
    }

    uint32_t level_size = count;
    hash_t *levels = (hash_t *)_alloca(count * sizeof(hash_t));
    for (uint32_t i = 0; i < count; i++)
    {
        levels[i] = txs[i]->tx_hash;
    }

    while (level_size > 1)
    {
        for (uint32_t i = 0; i < level_size / 2; i++)
        {
            uint8_t combined[HASH_LEN * 2];
            memcpy(combined, &levels[i * 2], HASH_LEN);
            memcpy(combined + HASH_LEN, &levels[i * 2 + 1], HASH_LEN);
            levels[i] = compute_hash_fast(combined, HASH_LEN * 2);
        }
        if (level_size & 1)
        {
            levels[level_size / 2] = levels[level_size - 1];
            level_size = level_size / 2 + 1;
        }
        else
        {
            level_size /= 2;
        }
    }
    return levels[0];
}

static balance_cache_t *balance_cache_create(size_t capacity)
{
    balance_cache_t *cache = (balance_cache_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(balance_cache_t));
    cache->capacity = capacity;
    cache->count = 0;
    cache->entries = (balance_entry_t *)aligned_alloc_custom(PAGE_SIZE, capacity * sizeof(balance_entry_t));
    cache->index_map = (uint32_t *)calloc(capacity, sizeof(uint32_t));
    cache->cache_hits = 0;
    cache->cache_misses = 0;
    InitializeCriticalSection(&cache->lock);
    memset(cache->entries, 0, capacity * sizeof(balance_entry_t));
    return cache;
}

static void balance_cache_destroy(balance_cache_t *cache)
{
    if (cache)
    {
        DeleteCriticalSection(&cache->lock);
        aligned_free_custom(cache->entries);
        free(cache->index_map);
        aligned_free_custom(cache);
    }
}

static void balance_cache_update(balance_cache_t *cache, const address_t *addr, token_id_t token_id, amount_t delta)
{
    EnterCriticalSection(&cache->lock);
    uint32_t idx = (addr->bytes[0] ^ (addr->bytes[1] << 8) ^ token_id) % cache->capacity;
    uint32_t start = idx;
    bool found = false;

    do
    {
        if (cache->entries[idx].last_update_block == 0)
        {
            cache->entries[idx].address = *addr;
            cache->entries[idx].token_id = token_id;
            cache->entries[idx].balance = delta;
            cache->entries[idx].last_update_block = 1;
            cache->entries[idx].last_transaction_nonce = 0;
            cache->entries[idx].locked_balance = 0;
            cache->entries[idx].lock_until_block = 0;
            cache->count++;
            found = true;
            break;
        }
        if (memcmp(&cache->entries[idx].address, addr, ADDRESS_LEN) == 0 &&
            cache->entries[idx].token_id == token_id)
        {
            cache->entries[idx].balance += delta;
            found = true;
            break;
        }
        idx = (idx + 1) % cache->capacity;
    } while (idx != start);

    if (found)
    {
        cache->cache_hits++;
    }
    else
    {
        cache->cache_misses++;
    }
    LeaveCriticalSection(&cache->lock);
}

static amount_t balance_cache_get(balance_cache_t *cache, const address_t *addr, token_id_t token_id)
{
    EnterCriticalSection(&cache->lock);
    uint32_t idx = (addr->bytes[0] ^ (addr->bytes[1] << 8) ^ token_id) % cache->capacity;
    uint32_t start = idx;
    amount_t result = 0;
    bool found = false;

    do
    {
        prefetch(&cache->entries[idx]);
        if (memcmp(&cache->entries[idx].address, addr, ADDRESS_LEN) == 0 &&
            cache->entries[idx].token_id == token_id)
        {
            result = cache->entries[idx].balance;
            found = true;
            break;
        }
        idx = (idx + 1) % cache->capacity;
    } while (idx != start && cache->entries[idx].last_update_block != 0);

    if (found)
    {
        cache->cache_hits++;
    }
    else
    {
        cache->cache_misses++;
    }
    LeaveCriticalSection(&cache->lock);
    return result;
}

static transaction_pool_t *transaction_pool_create(uint64_t capacity)
{
    transaction_pool_t *pool = (transaction_pool_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(transaction_pool_t));
    pool->capacity = capacity;
    pool->pool = (transaction_t *)aligned_alloc_custom(PAGE_SIZE, capacity * sizeof(transaction_t));
    pool->head = 0;
    pool->tail = 0;
    pool->size = 0;
    pool->total_added = 0;
    pool->total_removed = 0;
    pool->wait_events = (HANDLE *)malloc(sizeof(HANDLE));
    pool->wait_events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&pool->lock);
    return pool;
}

static void transaction_pool_destroy(transaction_pool_t *pool)
{
    if (pool)
    {
        DeleteCriticalSection(&pool->lock);
        CloseHandle(pool->wait_events[0]);
        free(pool->wait_events);
        aligned_free_custom(pool->pool);
        aligned_free_custom(pool);
    }
}

static bool transaction_pool_add(transaction_pool_t *pool, const transaction_t *tx)
{
    EnterCriticalSection(&pool->lock);
    if (pool->size >= pool->capacity)
    {
        LeaveCriticalSection(&pool->lock);
        return false;
    }
    uint64_t idx = pool->tail % pool->capacity;
    memcpy(&pool->pool[idx], tx, sizeof(transaction_t));
    pool->tail++;
    pool->size++;
    pool->total_added++;
    SetEvent(pool->wait_events[0]);
    LeaveCriticalSection(&pool->lock);
    return true;
}

static transaction_t *transaction_pool_pop(transaction_pool_t *pool)
{
    EnterCriticalSection(&pool->lock);
    if (pool->size == 0)
    {
        LeaveCriticalSection(&pool->lock);
        return NULL;
    }
    uint64_t idx = pool->head % pool->capacity;
    transaction_t *tx = (transaction_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(transaction_t));
    memcpy(tx, &pool->pool[idx], sizeof(transaction_t));
    pool->head++;
    pool->size--;
    pool->total_removed++;
    LeaveCriticalSection(&pool->lock);
    return tx;
}

static transaction_t *transaction_pool_peek(transaction_pool_t *pool)
{
    EnterCriticalSection(&pool->lock);
    if (pool->size == 0)
    {
        LeaveCriticalSection(&pool->lock);
        return NULL;
    }
    uint64_t idx = pool->head % pool->capacity;
    transaction_t *tx = (transaction_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(transaction_t));
    memcpy(tx, &pool->pool[idx], sizeof(transaction_t));
    LeaveCriticalSection(&pool->lock);
    return tx;
}

static bool transaction_pool_remove_by_hash(transaction_pool_t *pool, const hash_t *tx_hash)
{
    EnterCriticalSection(&pool->lock);
    uint64_t current = pool->head;
    bool found = false;

    while (current < pool->tail)
    {
        uint64_t idx = current % pool->capacity;
        if (memcmp(&pool->pool[idx].tx_hash, tx_hash, HASH_LEN) == 0)
        {
            found = true;
            break;
        }
        current++;
    }
    LeaveCriticalSection(&pool->lock);
    return found;
}

static blockchain_t *blockchain_create(void)
{
    blockchain_t *chain = (blockchain_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(blockchain_t));
    chain->blocks = (block_t **)aligned_alloc_custom(PAGE_SIZE, MAX_BLOCKS * sizeof(block_t *));
    chain->block_index = (hash_t *)aligned_alloc_custom(PAGE_SIZE, MAX_BLOCKS * sizeof(hash_t));
    chain->difficulty_target = (uint32_t *)aligned_alloc_custom(PAGE_SIZE, MAX_BLOCKS * sizeof(uint32_t));
    chain->block_timestamps = (uint64_t *)aligned_alloc_custom(PAGE_SIZE, MAX_BLOCKS * sizeof(uint64_t));
    chain->block_hashes_by_height = (hash_t *)aligned_alloc_custom(PAGE_SIZE, MAX_BLOCKS * sizeof(hash_t));
    chain->height = 0;
    chain->total_difficulty = 0;
    chain->prune_height = 0;
    InitializeCriticalSection(&chain->chain_lock);
    InitializeCriticalSection(&chain->prune_lock);

    block_t *genesis = (block_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(block_t));
    memset(genesis, 0, sizeof(block_t));
    genesis->height = 0;
    genesis->timestamp = time(NULL);
    genesis->tx_count = 0;
    genesis->difficulty = 0x0000FFFF;
    genesis->reward = 5000000000ULL;
    genesis->version = 1;
    genesis->gas_limit = 8000000;
    genesis->block_hash = compute_hash((uint8_t *)genesis, sizeof(block_t));

    chain->blocks[0] = genesis;
    chain->block_hashes_by_height[0] = genesis->block_hash;
    chain->height = 1;
    chain->total_difficulty = genesis->difficulty;

    return chain;
}

static void blockchain_destroy(blockchain_t *chain)
{
    if (chain)
    {
        for (block_height_t i = 0; i < chain->height; i++)
        {
            if (chain->blocks[i])
            {
                aligned_free_custom(chain->blocks[i]);
            }
        }
        DeleteCriticalSection(&chain->chain_lock);
        DeleteCriticalSection(&chain->prune_lock);
        aligned_free_custom(chain->blocks);
        aligned_free_custom(chain->block_index);
        aligned_free_custom(chain->difficulty_target);
        aligned_free_custom(chain->block_timestamps);
        aligned_free_custom(chain->block_hashes_by_height);
        aligned_free_custom(chain);
    }
}

static bool blockchain_add_block(blockchain_t *chain, block_t *block)
{
    EnterCriticalSection(&chain->chain_lock);
    if (chain->height >= MAX_BLOCKS)
    {
        LeaveCriticalSection(&chain->chain_lock);
        return false;
    }

    block->height = chain->height;
    chain->blocks[chain->height] = block;
    chain->block_hashes_by_height[chain->height] = block->block_hash;
    chain->block_timestamps[chain->height] = block->timestamp;
    chain->total_difficulty += block->difficulty;
    chain->height++;

    LeaveCriticalSection(&chain->chain_lock);
    return true;
}

static block_t *blockchain_get_block(blockchain_t *chain, block_height_t height)
{
    EnterCriticalSection(&chain->chain_lock);
    if (height >= chain->height)
    {
        LeaveCriticalSection(&chain->chain_lock);
        return NULL;
    }
    block_t *block = chain->blocks[height];
    LeaveCriticalSection(&chain->chain_lock);
    return block;
}

static token_registry_t *token_registry_create(void)
{
    token_registry_t *registry = (token_registry_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(token_registry_t));
    registry->tokens = (token_t *)aligned_alloc_custom(PAGE_SIZE, MAX_TOKENS * sizeof(token_t));
    registry->token_hashes = (hash_t *)aligned_alloc_custom(PAGE_SIZE, MAX_TOKENS * sizeof(hash_t));
    registry->token_id_map = (uint32_t *)calloc(MAX_TOKENS, sizeof(uint32_t));
    registry->token_symbol_map = (uint32_t *)calloc(MAX_TOKENS, sizeof(uint32_t));
    registry->token_volumes = (amount_t *)calloc(MAX_TOKENS, sizeof(amount_t));
    registry->token_holders_count = (uint32_t *)calloc(MAX_TOKENS, sizeof(uint32_t));
    registry->token_count = 0;
    InitializeCriticalSection(&registry->registry_lock);
    return registry;
}

static void token_registry_destroy(token_registry_t *registry)
{
    if (registry)
    {
        DeleteCriticalSection(&registry->registry_lock);
        aligned_free_custom(registry->tokens);
        aligned_free_custom(registry->token_hashes);
        free(registry->token_id_map);
        free(registry->token_symbol_map);
        free(registry->token_volumes);
        free(registry->token_holders_count);
        aligned_free_custom(registry);
    }
}

static token_id_t token_registry_register(token_registry_t *registry, const token_t *token)
{
    EnterCriticalSection(&registry->registry_lock);
    if (registry->token_count >= MAX_TOKENS)
    {
        LeaveCriticalSection(&registry->registry_lock);
        return (token_id_t)-1;
    }
    token_id_t id = registry->token_count;
    registry->tokens[id] = *token;
    registry->tokens[id].id = id;
    registry->token_hashes[id] = compute_hash((uint8_t *)token, sizeof(token_t));
    registry->token_id_map[id] = id;
    registry->token_count++;
    LeaveCriticalSection(&registry->registry_lock);
    return id;
}

static peer_manager_t *peer_manager_create(void)
{
    peer_manager_t *pm = (peer_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(peer_manager_t));
    memset(pm->peers, 0, sizeof(pm->peers));
    pm->peer_count = 0;
    pm->active_connections = 0;
    pm->connection_ranks = (peer_id_t *)aligned_alloc_custom(PAGE_SIZE, MAX_PEERS * sizeof(peer_id_t));
    pm->peer_bandwidth = (uint32_t *)calloc(MAX_PEERS, sizeof(uint32_t));
    pm->peer_events = (HANDLE *)malloc(MAX_PEERS * sizeof(HANDLE));
    for (int i = 0; i < MAX_PEERS; i++)
    {
        pm->peer_events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    InitializeCriticalSection(&pm->peer_lock);
    return pm;
}

static void peer_manager_destroy(peer_manager_t *pm)
{
    if (pm)
    {
        DeleteCriticalSection(&pm->peer_lock);
        for (int i = 0; i < MAX_PEERS; i++)
        {
            CloseHandle(pm->peer_events[i]);
        }
        free(pm->peer_events);
        free(pm->peer_bandwidth);
        aligned_free_custom(pm->connection_ranks);
        aligned_free_custom(pm);
    }
}

static bool peer_manager_add(peer_manager_t *pm, const char *ip, uint16_t port)
{
    EnterCriticalSection(&pm->peer_lock);
    if (pm->peer_count >= MAX_PEERS)
    {
        LeaveCriticalSection(&pm->peer_lock);
        return false;
    }
    peer_t *peer = &pm->peers[pm->peer_count];
    peer->id = pm->peer_count;
    strncpy(peer->ip, ip, 15);
    peer->port = port;
    peer->last_seen = time(NULL);
    peer->score = 100;
    peer->is_active = true;
    peer->connected_peers = 0;
    peer->bandwidth_used = 0;
    peer->blocks_sent = 0;
    peer->blocks_received = 0;
    peer->fail_count = 0;
    peer->reputation = 100;
    InitializeCriticalSection(&peer->peer_lock);
    pm->peer_count++;
    pm->active_connections++;
    LeaveCriticalSection(&pm->peer_lock);
    return true;
}

static staking_manager_t *staking_manager_create(void)
{
    staking_manager_t *sm = (staking_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(staking_manager_t));
    memset(sm->validators, 0, sizeof(sm->validators));
    sm->validator_count = 0;
    sm->total_staked = 0;
    sm->validator_ranks = (uint32_t *)calloc(MAX_STAKING_VALIDATORS, sizeof(uint32_t));
    sm->delegation_pools = (amount_t *)calloc(MAX_STAKING_VALIDATORS, sizeof(amount_t));
    sm->min_stake_amount = 10000;
    sm->epoch_duration = 86400;
    sm->current_epoch = 0;
    InitializeCriticalSection(&sm->staking_lock);
    return sm;
}

static void staking_manager_destroy(staking_manager_t *sm)
{
    if (sm)
    {
        DeleteCriticalSection(&sm->staking_lock);
        free(sm->validator_ranks);
        free(sm->delegation_pools);
        aligned_free_custom(sm);
    }
}

static bool staking_manager_register_validator(staking_manager_t *sm, const address_t *addr, amount_t stake, uint32_t commission)
{
    EnterCriticalSection(&sm->staking_lock);
    if (sm->validator_count >= MAX_STAKING_VALIDATORS)
    {
        LeaveCriticalSection(&sm->staking_lock);
        return false;
    }
    validator_t *val = &sm->validators[sm->validator_count];
    val->id = sm->validator_count;
    val->address = *addr;
    val->stake = stake;
    val->rewards = 0;
    val->blocks_validated = 0;
    val->missed_blocks = 0;
    val->uptime_percent = 100;
    val->registered_at = time(NULL);
    val->is_active = true;
    val->commission_bps = commission;
    val->reward_address = *addr;
    val->version = 1;
    sm->total_staked += stake;
    sm->validator_count++;
    LeaveCriticalSection(&sm->staking_lock);
    return true;
}

static governance_manager_t *governance_manager_create(void)
{
    governance_manager_t *gm = (governance_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(governance_manager_t));
    memset(gm->proposals, 0, sizeof(gm->proposals));
    gm->proposal_count = 0;
    gm->min_voting_power = 1000000;
    gm->voting_delay_blocks = 100;
    gm->voting_period_blocks = 1000;
    InitializeCriticalSection(&gm->governance_lock);
    return gm;
}

static void governance_manager_destroy(governance_manager_t *gm)
{
    if (gm)
    {
        DeleteCriticalSection(&gm->governance_lock);
        aligned_free_custom(gm);
    }
}

static proposal_id_t governance_manager_create_proposal(governance_manager_t *gm, const address_t *proposer, const char *title, const char *description, uint8_t proposal_type)
{
    EnterCriticalSection(&gm->governance_lock);
    if (gm->proposal_count >= MAX_GOVERNANCE_PROPOSALS)
    {
        LeaveCriticalSection(&gm->governance_lock);
        return (proposal_id_t)-1;
    }
    proposal_id_t id = gm->proposal_count;
    governance_proposal_t *prop = &gm->proposals[id];
    prop->id = id;
    prop->proposer = *proposer;
    strncpy(prop->title, title, 255);
    strncpy(prop->description, description, 4095);
    prop->start_epoch = gm->proposal_count;
    prop->end_epoch = prop->start_epoch + 100;
    prop->for_votes = 0;
    prop->against_votes = 0;
    prop->abstain_votes = 0;
    prop->status = 0;
    prop->proposal_type = proposal_type;
    gm->proposal_count++;
    LeaveCriticalSection(&gm->governance_lock);
    return id;
}

static liquidity_manager_t *liquidity_manager_create(void)
{
    liquidity_manager_t *lm = (liquidity_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(liquidity_manager_t));
    memset(lm->pools, 0, sizeof(lm->pools));
    lm->pool_count = 0;
    lm->pool_slippages = (uint64_t *)calloc(MAX_LIQUIDITY_POOLS, sizeof(uint64_t));
    lm->pool_apr = (amount_t *)calloc(MAX_LIQUIDITY_POOLS, sizeof(amount_t));
    InitializeCriticalSection(&lm->pool_lock);
    return lm;
}

static void liquidity_manager_destroy(liquidity_manager_t *lm)
{
    if (lm)
    {
        DeleteCriticalSection(&lm->pool_lock);
        free(lm->pool_slippages);
        free(lm->pool_apr);
        aligned_free_custom(lm);
    }
}

static pool_id_t liquidity_manager_create_pool(liquidity_manager_t *lm, const address_t *token_a, const address_t *token_b, uint32_t fee_bps)
{
    EnterCriticalSection(&lm->pool_lock);
    if (lm->pool_count >= MAX_LIQUIDITY_POOLS)
    {
        LeaveCriticalSection(&lm->pool_lock);
        return (pool_id_t)-1;
    }
    pool_id_t id = lm->pool_count;
    liquidity_pool_t *pool = &lm->pools[id];
    pool->id = id;
    pool->token_a = *token_a;
    pool->token_b = *token_b;
    pool->reserve_a = 0;
    pool->reserve_b = 0;
    pool->total_liquidity = 0;
    pool->fee_bps = fee_bps;
    pool->created_at = time(NULL);
    pool->volume_24h = 0;
    pool->is_active = true;
    lm->pool_count++;
    LeaveCriticalSection(&lm->pool_lock);
    return id;
}

static amount_t liquidity_manager_calculate_swap(liquidity_manager_t *lm, pool_id_t id, amount_t amount_in, bool is_token_a)
{
    EnterCriticalSection(&lm->pool_lock);
    if (id >= lm->pool_count)
    {
        LeaveCriticalSection(&lm->pool_lock);
        return 0;
    }
    liquidity_pool_t *pool = &lm->pools[id];
    amount_t reserve_in = is_token_a ? pool->reserve_a : pool->reserve_b;
    amount_t reserve_out = is_token_a ? pool->reserve_b : pool->reserve_a;

    if (reserve_in == 0 || reserve_out == 0)
    {
        LeaveCriticalSection(&lm->pool_lock);
        return 0;
    }

    amount_t amount_in_with_fee = amount_in * (10000 - pool->fee_bps) / 10000;
    amount_t amount_out = (amount_in_with_fee * reserve_out) / (reserve_in + amount_in_with_fee);

    LeaveCriticalSection(&lm->pool_lock);
    return amount_out;
}

static nft_manager_t *nft_manager_create(void)
{
    nft_manager_t *nm = (nft_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(nft_manager_t));
    memset(nm->collections, 0, sizeof(nm->collections));
    memset(nm->items, 0, sizeof(nm->items));
    nm->collection_count = 0;
    nm->item_count = 0;
    nm->collection_index_map = (uint32_t *)calloc(MAX_NFT_COLLECTIONS, sizeof(uint32_t));
    nm->nft_owner_index = (uint64_t *)calloc(MAX_NFT_ITEMS, sizeof(uint64_t));
    InitializeCriticalSection(&nm->nft_lock);
    return nm;
}

static void nft_manager_destroy(nft_manager_t *nm)
{
    if (nm)
    {
        DeleteCriticalSection(&nm->nft_lock);
        free(nm->collection_index_map);
        free(nm->nft_owner_index);
        aligned_free_custom(nm);
    }
}

static collection_id_t nft_manager_create_collection(nft_manager_t *nm, const char *name, const char *symbol, const address_t *owner, uint32_t max_supply)
{
    EnterCriticalSection(&nm->nft_lock);
    if (nm->collection_count >= MAX_NFT_COLLECTIONS)
    {
        LeaveCriticalSection(&nm->nft_lock);
        return (collection_id_t)-1;
    }
    collection_id_t id = nm->collection_count;
    nft_collection_t *col = &nm->collections[id];
    col->id = id;
    strncpy(col->name, name, 63);
    strncpy(col->symbol, symbol, 15);
    col->owner = *owner;
    col->total_supply = 0;
    col->max_supply = max_supply;
    col->created_at = time(NULL);
    col->is_frozen = false;
    col->royalty_bps = 250;
    col->royalty_recipient = *owner;
    nm->collection_count++;
    LeaveCriticalSection(&nm->nft_lock);
    return id;
}

static nft_id_t nft_manager_mint(nft_manager_t *nm, collection_id_t collection_id, const address_t *to, const hash_t *metadata)
{
    EnterCriticalSection(&nm->nft_lock);
    if (collection_id >= nm->collection_count)
    {
        LeaveCriticalSection(&nm->nft_lock);
        return (nft_id_t)-1;
    }
    if (nm->item_count >= MAX_NFT_ITEMS)
    {
        LeaveCriticalSection(&nm->nft_lock);
        return (nft_id_t)-1;
    }
    nft_collection_t *col = &nm->collections[collection_id];
    if (col->total_supply >= col->max_supply)
    {
        LeaveCriticalSection(&nm->nft_lock);
        return (nft_id_t)-1;
    }
    nft_id_t id = nm->item_count;
    nft_item_t *item = &nm->items[id];
    item->id = id;
    item->collection_id = collection_id;
    item->owner = *to;
    item->token_id = col->total_supply + 1;
    item->metadata_hash = *metadata;
    item->minted_at = time(NULL);
    item->last_sale_price = 0;
    item->transfer_count = 0;
    item->is_listed = false;
    item->listed_price = 0;
    item->creator = *to;
    item->royalty_paid = 0;
    col->total_supply++;
    nm->item_count++;
    LeaveCriticalSection(&nm->nft_lock);
    return id;
}

static atomic_swap_manager_t *atomic_swap_manager_create_real(void)
{
    atomic_swap_manager_t *asm = (atomic_swap_manager_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(atomic_swap_manager_t));
    memset(asm->swaps, 0, sizeof(asm->swaps));
    asm->swap_count = 0;
    asm->swap_index = (hash_t *)calloc(MAX_ATOMIC_SWAPS, sizeof(hash_t));
    asm->swap_timers = (HANDLE *)malloc(MAX_ATOMIC_SWAPS * sizeof(HANDLE));
    InitializeCriticalSection(&asm->swap_lock);
    return asm;
}

static void atomic_swap_manager_destroy(atomic_swap_manager_t *asm)
{
    if (asm)
    {
        DeleteCriticalSection(&asm->swap_lock);
        free(asm->swap_index);
        free(asm->swap_timers);
        aligned_free_custom(asm);
    }
}

static hash_t atomic_swap_manager_create(atomic_swap_manager_t *asm, const address_t *initiator, const address_t *asset_a, const address_t *asset_b, amount_t amount_a, amount_t amount_b, const hash_t *secret_hash, timestamp_t timeout)
{
    EnterCriticalSection(&asm->swap_lock);
    if (asm->swap_count >= MAX_ATOMIC_SWAPS)
    {
        LeaveCriticalSection(&asm->swap_lock);
        hash_t empty = {0};
        return empty;
    }
    atomic_swap_t *swap = &asm->swaps[asm->swap_count];
    swap->initiator = *initiator;
    swap->asset_a = *asset_a;
    swap->asset_b = *asset_b;
    swap->amount_a = amount_a;
    swap->amount_b = amount_b;
    swap->secret_hash = *secret_hash;
    swap->timeout = timeout;
    swap->status = 0;
    swap->version = 1;

    uint8_t combined[sizeof(address_t) * 4 + sizeof(amount_t) * 2 + sizeof(hash_t)];
    memcpy(combined, initiator, ADDRESS_LEN);
    memcpy(combined + ADDRESS_LEN, asset_a, ADDRESS_LEN);
    memcpy(combined + ADDRESS_LEN * 2, asset_b, ADDRESS_LEN);
    memcpy(combined + ADDRESS_LEN * 3, &amount_a, sizeof(amount_t));
    memcpy(combined + ADDRESS_LEN * 3 + sizeof(amount_t), &amount_b, sizeof(amount_t));
    memcpy(combined + ADDRESS_LEN * 3 + sizeof(amount_t) * 2, secret_hash, HASH_LEN);
    swap->swap_id = compute_hash(combined, sizeof(combined));
    asm->swap_index[asm->swap_count] = swap->swap_id;
    asm->swap_count++;
    LeaveCriticalSection(&asm->swap_lock);
    return swap->swap_id;
}

static uint8_t atomic_swap_manager_get_status(atomic_swap_manager_t *asm, const hash_t *swap_id)
{
    EnterCriticalSection(&asm->swap_lock);
    for (uint32_t i = 0; i < asm->swap_count; i++)
    {
        if (memcmp(&asm->swaps[i].swap_id, swap_id, HASH_LEN) == 0)
        {
            uint8_t status = asm->swaps[i].status;
            LeaveCriticalSection(&asm->swap_lock);
            return status;
        }
    }
    LeaveCriticalSection(&asm->swap_lock);
    return 0xFF;
}

static p2p_engine_t *p2p_engine_create(uint32_t worker_threads)
{
    p2p_engine_t *engine = (p2p_engine_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(p2p_engine_t));
    engine->tx_pool = transaction_pool_create(TRANSACTION_POOL_SIZE);
    engine->blockchain = blockchain_create();
    engine->token_registry = token_registry_create();
    engine->balance_cache = balance_cache_create(5000000);
    engine->peer_manager = peer_manager_create();
    engine->running = true;
    engine->worker_count = worker_threads > 64 ? 64 : worker_threads;
    engine->total_transactions_processed = 0;
    engine->total_blocks_mined = 0;
    engine->total_peers_connected = 0;
    engine->start_time = time(NULL);
    engine->thread_affinities = (uint32_t *)calloc(worker_threads, sizeof(uint32_t));
    engine->target_block_time_ms = 10000;
    engine->current_total_peers = 0;
    InitializeCriticalSection(&engine->metrics_lock);

    for (uint32_t i = 0; i < engine->worker_count; i++)
    {
        engine->thread_affinities[i] = i % 64;
    }
    return engine;
}

static void p2p_engine_destroy(p2p_engine_t *engine)
{
    if (engine)
    {
        transaction_pool_destroy(engine->tx_pool);
        blockchain_destroy(engine->blockchain);
        token_registry_destroy(engine->token_registry);
        balance_cache_destroy(engine->balance_cache);
        peer_manager_destroy(engine->peer_manager);
        DeleteCriticalSection(&engine->metrics_lock);
        free(engine->thread_affinities);
        aligned_free_custom(engine);
    }
}

static void compute_transaction_hash(transaction_t *tx)
{
    uint8_t buffer[sizeof(transaction_t) - sizeof(hash_t) - sizeof(hash_t)];
    memcpy(buffer, &tx->from, ADDRESS_LEN);
    memcpy(buffer + ADDRESS_LEN, &tx->to, ADDRESS_LEN);
    memcpy(buffer + ADDRESS_LEN * 2, &tx->amount, sizeof(amount_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t), &tx->token_id, sizeof(token_id_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t) + sizeof(token_id_t), &tx->timestamp, sizeof(timestamp_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t) + sizeof(token_id_t) + sizeof(timestamp_t), &tx->nonce, sizeof(nonce_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t) + sizeof(token_id_t) + sizeof(timestamp_t) + sizeof(nonce_t), &tx->fee, sizeof(uint32_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t) + sizeof(token_id_t) + sizeof(timestamp_t) + sizeof(nonce_t) + sizeof(uint32_t), &tx->gas_price, sizeof(uint32_t));
    memcpy(buffer + ADDRESS_LEN * 2 + sizeof(amount_t) + sizeof(token_id_t) + sizeof(timestamp_t) + sizeof(nonce_t) + sizeof(uint32_t) * 2, &tx->gas_limit, sizeof(uint64_t));
    tx->tx_hash = compute_hash_fast(buffer, sizeof(buffer));
}

static bool verify_transaction(const transaction_t *tx, const balance_cache_t *cache, uint32_t current_block)
{
    amount_t balance = balance_cache_get((balance_cache_t *)cache, &tx->from, tx->token_id);
    amount_t total_cost = tx->amount + tx->fee + (tx->gas_price * tx->gas_limit / 1000000000);

    if (balance < total_cost)
    {
        return false;
    }

    if (tx->deadline > 0 && tx->deadline < current_block)
    {
        return false;
    }

    return true;
}

static bool process_transaction(p2p_engine_t *engine, transaction_t *tx)
{
    block_t *latest_block = blockchain_get_block(engine->blockchain, engine->blockchain->height - 1);
    uint32_t current_block = latest_block ? latest_block->height : 0;

    if (!verify_transaction(tx, engine->balance_cache, current_block))
    {
        return false;
    }

    amount_t total_cost = tx->amount + tx->fee + (tx->gas_price * tx->gas_limit / 1000000000);
    balance_cache_update(engine->balance_cache, &tx->from, tx->token_id, -total_cost);
    balance_cache_update(engine->balance_cache, &tx->to, tx->token_id, tx->amount);

    tx->is_processed = true;
    atomic_increment(&engine->total_transactions_processed);
    return true;
}

static block_t *create_block(p2p_engine_t *engine, const address_t *miner)
{
    block_t *block = (block_t *)aligned_alloc_custom(CACHE_LINE_SIZE, sizeof(block_t));
    memset(block, 0, sizeof(block_t));

    block->timestamp = time(NULL);
    block->height = engine->blockchain->height;
    block->tx_count = 0;
    block->miner = *miner;
    block->reward = 5000000000ULL;
    block->gas_used = 0;
    block->gas_limit = 8000000;
    block->version = 2;

    transaction_t *tx_pool[BLOCK_SIZE];
    uint32_t tx_fetched = 0;
    uint64_t total_gas = 0;

    for (uint32_t i = 0; i < BLOCK_SIZE && tx_fetched < BLOCK_SIZE; i++)
    {
        transaction_t *tx = transaction_pool_pop(engine->tx_pool);
        if (tx)
        {
            uint64_t tx_gas = tx->gas_limit;
            if (total_gas + tx_gas <= block->gas_limit)
            {
                tx_pool[tx_fetched++] = tx;
                total_gas += tx_gas;
            }
            else
            {
                transaction_pool_add(engine->tx_pool, tx);
                break;
            }
        }
        else
        {
            break;
        }
    }

    for (uint32_t i = 0; i < tx_fetched; i++)
    {
        block->transactions[i] = tx_pool[i];
        block->tx_count++;
    }

    if (block->tx_count > 0)
    {
        block->merkle_root = compute_merkle_root(block->transactions, block->tx_count);
        for (uint32_t i = 0; i < block->tx_count; i++)
        {
            block->gas_used += block->transactions[i]->gas_limit;
        }
    }

    if (engine->blockchain->height > 0)
    {
        block->prev_block_hash = engine->blockchain->blocks[engine->blockchain->height - 1]->block_hash;
        uint32_t prev_diff = engine->blockchain->blocks[engine->blockchain->height - 1]->difficulty;
        block->difficulty = prev_diff;
    }
    else
    {
        block->difficulty = 0x0000FFFF;
    }

    block->state_root = compute_hash_fast((uint8_t *)engine->balance_cache->entries, sizeof(balance_entry_t) * engine->balance_cache->count);
    block->receipts_root = compute_hash_fast((uint8_t *)block->transactions, sizeof(transaction_t *) * block->tx_count);

    return block;
}

static void mine_block(p2p_engine_t *engine, block_t *block)
{
    uint64_t start_time = rdtsc();
    uint32_t target = 0x0000FFFF / (block->difficulty ? block->difficulty : 1);
    if (target < 1)
        target = 1;

    for (uint32_t nonce = 0; nonce < 0xFFFFFFFF; nonce++)
    {
        block->nonce = nonce;
        block->block_hash = compute_hash_fast((uint8_t *)block, sizeof(block_t));

        if (*(uint32_t *)block->block_hash.bytes < target)
        {
            if (blockchain_add_block(engine->blockchain, block))
            {
                balance_cache_update(engine->balance_cache, &block->miner, 0, block->reward);
                atomic_increment(&engine->total_blocks_mined);

                for (uint32_t i = 0; i < block->tx_count; i++)
                {
                    if (process_transaction(engine, block->transactions[i]))
                    {
                        atomic_increment(&engine->total_transactions_processed);
                    }
                    free(block->transactions[i]);
                }
            }
            break;
        }

        if ((nonce & 0xFFFFF) == 0)
        {
            uint64_t now = rdtsc();
            if ((now - start_time) > 3000000000ULL)
            {
                break;
            }
        }
    }
}

static DWORD WINAPI worker_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;

    while (engine->running)
    {
        transaction_t *tx = transaction_pool_pop(engine->tx_pool);
        if (tx)
        {
            if (process_transaction(engine, tx))
            {
                atomic_increment(&engine->tx_pool->size);
            }
            free(tx);
        }
        else
        {
            Sleep(1);
        }
    }
    return 0;
}

static DWORD WINAPI consensus_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;
    address_t miner_addr = {0};
    miner_addr.bytes[0] = 0x01;

    while (engine->running)
    {
        if (engine->tx_pool->size >= 10)
        {
            block_t *block = create_block(engine, &miner_addr);
            mine_block(engine, block);
        }
        Sleep(engine->target_block_time_ms);
    }
    return 0;
}

static DWORD WINAPI network_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;

    while (engine->running)
    {
        EnterCriticalSection(&engine->peer_manager->peer_lock);
        for (uint32_t i = 0; i < engine->peer_manager->peer_count; i++)
        {
            peer_t *peer = &engine->peer_manager->peers[i];
            if (peer->is_active)
            {
                peer->latency_ms = (GetTickCount() % 100) + 1;
                peer->last_seen = time(NULL);
                peer->bandwidth_used += 1024 * (rand() % 100);
                engine->current_total_peers = engine->peer_manager->active_connections;
            }
        }
        LeaveCriticalSection(&engine->peer_manager->peer_lock);
        Sleep(100);
    }
    return 0;
}

static DWORD WINAPI cleanup_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;

    while (engine->running)
    {
        Sleep(60000);

        EnterCriticalSection(&engine->blockchain->prune_lock);
        if (engine->blockchain->height > 1000)
        {
            uint32_t new_prune = engine->blockchain->height - 1000;
            if (new_prune > engine->blockchain->prune_height)
            {
                for (block_height_t i = engine->blockchain->prune_height; i < new_prune; i++)
                {
                    if (engine->blockchain->blocks[i] && i > 0)
                    {
                        aligned_free_custom(engine->blockchain->blocks[i]);
                        engine->blockchain->blocks[i] = NULL;
                    }
                }
                engine->blockchain->prune_height = new_prune;
            }
        }
        LeaveCriticalSection(&engine->blockchain->prune_lock);
    }
    return 0;
}

static DWORD WINAPI metrics_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;

    while (engine->running)
    {
        Sleep(5000);

        EnterCriticalSection(&engine->metrics_lock);
        uint64_t tps = engine->total_transactions_processed / 5;
        uint64_t block_rate = engine->total_blocks_mined * 12;
        double uptime = difftime(time(NULL), engine->start_time);

        printf("\n=== P2P SYSTEM METRICS ===\n");
        printf("TPS: %llu\n", (unsigned long long)tps);
        printf("Blocks per minute: %llu\n", (unsigned long long)block_rate);
        printf("Pool size: %llu\n", (unsigned long long)engine->tx_pool->size);
        printf("Blockchain height: %u\n", engine->blockchain->height);
        printf("Active peers: %u\n", engine->current_total_peers);
        printf("Total tx processed: %llu\n", (unsigned long long)engine->total_transactions_processed);
        printf("Uptime: %.0f seconds\n", uptime);
        printf("Cache hit rate: %.2f%%\n",
               engine->balance_cache->cache_hits + engine->balance_cache->cache_misses > 0 ? (double)engine->balance_cache->cache_hits /
                                                                                                 (engine->balance_cache->cache_hits + engine->balance_cache->cache_misses) * 100
                                                                                           : 0);
        printf("==========================\n\n");

        LeaveCriticalSection(&engine->metrics_lock);
    }
    return 0;
}

static DWORD WINAPI validation_thread_function(LPVOID arg)
{
    p2p_engine_t *engine = (p2p_engine_t *)arg;

    while (engine->running)
    {
        Sleep(30000);

        block_t *last_block = blockchain_get_block(engine->blockchain, engine->blockchain->height - 1);
        if (last_block)
        {
            hash_t computed_hash = compute_hash_fast((uint8_t *)last_block, sizeof(block_t));
            if (memcmp(&computed_hash, &last_block->block_hash, HASH_LEN) != 0)
            {
                printf("CRITICAL: Blockchain validation failed at height %u\n", last_block->height);
            }
        }
    }
    return 0;
}

static void p2p_engine_start(p2p_engine_t *engine)
{
    for (uint32_t i = 0; i < engine->worker_count; i++)
    {
        engine->worker_threads[i] = CreateThread(NULL, 0, worker_thread_function, engine, 0, NULL);
        if (engine->worker_threads[i])
        {
            SetThreadPriority(engine->worker_threads[i], THREAD_PRIORITY_HIGHEST);
        }
    }
    engine->consensus_thread = CreateThread(NULL, 0, consensus_thread_function, engine, 0, NULL);
    engine->network_thread = CreateThread(NULL, 0, network_thread_function, engine, 0, NULL);
    engine->cleanup_thread = CreateThread(NULL, 0, cleanup_thread_function, engine, 0, NULL);
    engine->metrics_thread = CreateThread(NULL, 0, metrics_thread_function, engine, 0, NULL);
    engine->validation_thread = CreateThread(NULL, 0, validation_thread_function, engine, 0, NULL);
}

static void p2p_engine_stop(p2p_engine_t *engine)
{
    engine->running = false;

    for (uint32_t i = 0; i < engine->worker_count; i++)
    {
        if (engine->worker_threads[i])
        {
            WaitForSingleObject(engine->worker_threads[i], 5000);
            CloseHandle(engine->worker_threads[i]);
        }
    }
    if (engine->consensus_thread)
    {
        WaitForSingleObject(engine->consensus_thread, 5000);
        CloseHandle(engine->consensus_thread);
    }
    if (engine->network_thread)
    {
        WaitForSingleObject(engine->network_thread, 5000);
        CloseHandle(engine->network_thread);
    }
    if (engine->cleanup_thread)
    {
        WaitForSingleObject(engine->cleanup_thread, 5000);
        CloseHandle(engine->cleanup_thread);
    }
    if (engine->metrics_thread)
    {
        WaitForSingleObject(engine->metrics_thread, 5000);
        CloseHandle(engine->metrics_thread);
    }
    if (engine->validation_thread)
    {
        WaitForSingleObject(engine->validation_thread, 5000);
        CloseHandle(engine->validation_thread);
    }
}

static token_t create_token(const char *symbol, const char *name, const address_t *issuer, amount_t supply, uint8_t decimals)
{
    token_t token = {0};
    strncpy(token.symbol, symbol, TOKEN_SYMBOL_LEN - 1);
    strncpy(token.name, name, TOKEN_NAME_LEN - 1);
    token.issuer = *issuer;
    token.total_supply = supply;
    token.circulating_supply = supply;
    token.decimals = decimals;
    token.created_at = time(NULL);
    token.holders_count = 0;
    token.is_frozen = false;
    token.is_mintable = true;
    token.is_burnable = true;
    token.version = 1;
    token.max_supply = supply * 2;
    token.transfer_fee_bps = 0;
    token.fee_collector = *issuer;
    return token;
}

static bool transfer_tokens(p2p_engine_t *engine, const address_t *from, const address_t *to, token_id_t token_id, amount_t amount, uint32_t fee, uint32_t gas_price)
{
    transaction_t tx = {0};
    tx.from = *from;
    tx.to = *to;
    tx.amount = amount;
    tx.token_id = token_id;
    tx.timestamp = time(NULL);
    tx.nonce = rdtsc();
    tx.fee = fee;
    tx.gas_price = gas_price;
    tx.gas_limit = 21000;
    tx.version = 2;
    tx.tx_type = 0;
    tx.is_processed = false;

    compute_transaction_hash(&tx);

    return transaction_pool_add(engine->tx_pool, &tx);
}

static amount_t get_balance(p2p_engine_t *engine, const address_t *addr, token_id_t token_id)
{
    return balance_cache_get(engine->balance_cache, addr, token_id);
}

static transaction_status_t get_transaction_status(p2p_engine_t *engine, const hash_t *tx_hash)
{
    transaction_status_t status = {0};
    status.tx_hash = *tx_hash;
    status.timestamp = time(NULL);
    status.status = 0;
    status.confirmations = 0;
    status.gas_used = 0;
    status.fee_paid = 0;
    status.block_height = 0;

    EnterCriticalSection(&engine->blockchain->chain_lock);
    for (block_height_t i = 0; i < engine->blockchain->height; i++)
    {
        block_t *block = engine->blockchain->blocks[i];
        if (block)
        {
            for (uint32_t j = 0; j < block->tx_count; j++)
            {
                if (block->transactions[j] && memcmp(&block->transactions[j]->tx_hash, tx_hash, HASH_LEN) == 0)
                {
                    status.block_hash = block->block_hash;
                    status.confirmations = engine->blockchain->height - i;
                    status.status = 1;
                    status.block_height = i;
                    status.gas_used = block->transactions[j]->gas_limit;
                    status.fee_paid = block->transactions[j]->fee;
                    LeaveCriticalSection(&engine->blockchain->chain_lock);
                    return status;
                }
            }
        }
    }
    LeaveCriticalSection(&engine->blockchain->chain_lock);

    if (transaction_pool_remove_by_hash(engine->tx_pool, tx_hash))
    {
        status.status = 2;
    }

    return status;
}

static void print_system_stats(p2p_engine_t *engine)
{
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║                 P2P TOKEN SYSTEM v4.0 - STATISTICS               ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║ Blockchain Height:      %-40u ║\n", engine->blockchain->height);
    printf("║ Transaction Pool Size:  %-40llu ║\n", (unsigned long long)engine->tx_pool->size);
    printf("║ Active Peers:           %-40u ║\n", engine->current_total_peers);
    printf("║ Registered Tokens:      %-40u ║\n", engine->token_registry->token_count);
    printf("║ Balance Cache Entries:  %-40zu ║\n", engine->balance_cache->count);
    printf("║ Total TX Processed:     %-40llu ║\n", (unsigned long long)engine->total_transactions_processed);
    printf("║ Total Blocks Mined:     %-40llu ║\n", (unsigned long long)engine->total_blocks_mined);
    printf("║ Cache Hit Rate:         %-40.2f%% ║\n",
           engine->balance_cache->cache_hits + engine->balance_cache->cache_misses > 0 ? (double)engine->balance_cache->cache_hits /
                                                                                             (engine->balance_cache->cache_hits + engine->balance_cache->cache_misses) * 100
                                                                                       : 0);
    printf("║ Worker Threads:         %-40u ║\n", engine->worker_count);
    printf("║ Uptime (seconds):       %-40.0f ║\n", difftime(time(NULL), engine->start_time));
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
}

static void perform_stress_test(p2p_engine_t *engine, uint32_t duration_sec)
{
    printf("🚀 Starting STRESS TEST for %u seconds...\n", duration_sec);
    time_t start = time(NULL);
    uint64_t tx_counter = 0;
    uint64_t failed_counter = 0;

    address_t addresses[100];
    for (int i = 0; i < 100; i++)
    {
        memset(&addresses[i], i, ADDRESS_LEN);
        balance_cache_update(engine->balance_cache, &addresses[i], 0, 1000000000000ULL);
    }

    while (time(NULL) - start < duration_sec)
    {
        uint64_t batch_start = rdtsc();

        for (uint32_t i = 0; i < 10000; i++)
        {
            int from_idx = rand() % 100;
            int to_idx = rand() % 100;
            if (from_idx != to_idx)
            {
                amount_t amount = (rand() % 10000) + 1;
                if (transfer_tokens(engine, &addresses[from_idx], &addresses[to_idx], 0, amount, 1, 1))
                {
                    tx_counter++;
                }
                else
                {
                    failed_counter++;
                }
            }
        }

        uint64_t batch_end = rdtsc();
        uint64_t batch_time = (batch_end - batch_start) / 1000000;

        printf("Stress batch: %llu tx in %llu ms (%.0f tx/s)\n",
               (unsigned long long)10000, (unsigned long long)batch_time, 10000000.0 / batch_time);

        Sleep(100);
    }

    printf("\n✅ Stress test completed!\n");
    printf("   Total transactions: %llu\n", (unsigned long long)tx_counter);
    printf("   Failed transactions: %llu\n", (unsigned long long)failed_counter);
    printf("   Success rate: %.2f%%\n", (double)tx_counter / (tx_counter + failed_counter) * 100);
}

static void demonstrate_dex_functionality(p2p_engine_t *engine)
{
    printf("\n🏦 Initializing DEX (Decentralized Exchange)...\n");

    liquidity_manager_t *dex = liquidity_manager_create();

    address_t token_a = {0};
    address_t token_b = {0};
    token_a.bytes[0] = 0xAA;
    token_b.bytes[0] = 0xBB;

    pool_id_t pool_id = liquidity_manager_create_pool(dex, &token_a, &token_b, 30);
    printf("✅ Liquidity pool created with ID: %u (0.3%% fee)\n", pool_id);

    liquidity_pool_t *pool = &dex->pools[pool_id];
    pool->reserve_a = 100000000000000ULL;
    pool->reserve_b = 50000000000000ULL;
    pool->total_liquidity = (amount_t)sqrt((double)pool->reserve_a * (double)pool->reserve_b);

    amount_t swap_in = 1000000000000ULL;
    amount_t expected_out = liquidity_manager_calculate_swap(dex, pool_id, swap_in, true);
    printf("💱 Swap simulation: %llu token A -> ~%llu token B\n",
           (unsigned long long)swap_in, (unsigned long long)expected_out);

    liquidity_manager_destroy(dex);
}

static void demonstrate_nft_functionality(p2p_engine_t *engine)
{
    printf("\n🎨 Initializing NFT Marketplace...\n");

    nft_manager_t *nft = nft_manager_create();

    address_t owner = {0};
    owner.bytes[0] = 0xDE;
    owner.bytes[1] = 0xAD;

    collection_id_t collection = nft_manager_create_collection(nft, "CyberPunks", "PUNK", &owner, 10000);
    printf("✅ NFT collection created: ID=%u, Name=CyberPunks\n", collection);

    hash_t metadata = {0};
    memcpy(metadata.bytes, "ipfs://QmXxxCyberPunk1", 22);

    nft_id_t nft_item = nft_manager_mint(nft, collection, &owner, &metadata);
    printf("✅ NFT minted: ID=%llu in collection %u\n", (unsigned long long)nft_item, collection);

    nft_manager_destroy(nft);
}

static void demonstrate_swap_functionality(p2p_engine_t *engine)
{
    printf("\n🔄 Initializing Atomic Swap System...\n");

    atomic_swap_manager_t *swaps = atomic_swap_manager_create_real();

    address_t alice = {0};
    address_t bob = {0};
    address_t btc = {0};
    address_t eth = {0};
    alice.bytes[0] = 0xA1;
    bob.bytes[0] = 0xB2;
    btc.bytes[0] = 0x01;
    eth.bytes[0] = 0x02;

    hash_t secret_hash = {0};
    memcpy(secret_hash.bytes, "SecretHash123456789", 19);

    timestamp_t timeout = time(NULL) + 3600;
    hash_t swap_id = atomic_swap_manager_create(swaps, &alice, &btc, &eth, 100000000, 5000000000ULL, &secret_hash, timeout);

    printf("✅ Atomic swap created: ID=");
    for (int i = 0; i < 8; i++)
        printf("%02x", swap_id.bytes[i]);
    printf("...\n");

    uint8_t status = atomic_swap_manager_get_status(swaps, &swap_id);
    printf("📊 Swap status: %u (0=active)\n", status);

    atomic_swap_manager_destroy(swaps);
}

static void demonstrate_staking_functionality(p2p_engine_t *engine)
{
    printf("\n💰 Initializing Staking System...\n");

    staking_manager_t *staking = staking_manager_create();

    address_t validator_addr = {0};
    validator_addr.bytes[0] = 0x77;

    if (staking_manager_register_validator(staking, &validator_addr, 1000000, 500))
    {
        printf("✅ Validator registered with 1,000,000 stake (5%% commission)\n");
    }

    printf("📊 Total staked: %llu\n", (unsigned long long)staking->total_staked);

    staking_manager_destroy(staking);
}

static void demonstrate_governance_functionality(p2p_engine_t *engine)
{
    printf("\n🗳️ Initializing Governance System...\n");

    governance_manager_t *gov = governance_manager_create();

    address_t proposer = {0};
    proposer.bytes[0] = 0x99;

    proposal_id_t prop_id = governance_manager_create_proposal(gov, &proposer,
                                                               "Increase Block Size",
                                                               "Proposal to increase maximum block size from 8KB to 16KB for better scalability",
                                                               1);

    printf("✅ Governance proposal created: ID=%u\n", prop_id);
    printf("📋 Proposal: 'Increase Block Size'\n");

    governance_manager_destroy(gov);
}

int main(void)
{
    system("chcp 65001 > nul");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║         P2P TOKEN SYSTEM v4.0 - ULTIMATE EDITION                ║\n");
    printf("║                    High-Performance P2P Network                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    srand((unsigned int)time(NULL));

    printf("🔧 Initializing system components...\n");
    p2p_engine_t *engine = p2p_engine_create(16);

    printf("🚀 Starting engine with %u worker threads...\n", engine->worker_count);
    p2p_engine_start(engine);

    printf("📝 Registering main token...\n");
    address_t issuer = {0};
    issuer.bytes[0] = 0xFF;
    issuer.bytes[1] = 0xEE;

    token_t main_token = create_token("P2P", "PeerToPeer Token", &issuer, 10000000000ULL, 18);
    token_id_t main_token_id = token_registry_register(engine->token_registry, &main_token);
    printf("✅ Main token registered with ID: %u (10B supply, 18 decimals)\n", main_token_id);

    token_t stable_token = create_token("USDP", "USD Pegged", &issuer, 5000000000ULL, 6);
    token_id_t stable_token_id = token_registry_register(engine->token_registry, &stable_token);
    printf("✅ Stable token registered with ID: %u\n", stable_token_id);

    printf("🌐 Adding seed peers...\n");
    peer_manager_add(engine->peer_manager, "192.168.1.1", 8333);
    peer_manager_add(engine->peer_manager, "192.168.1.2", 8333);
    peer_manager_add(engine->peer_manager, "10.0.0.1", 8333);
    peer_manager_add(engine->peer_manager, "10.0.0.2", 8333);
    peer_manager_add(engine->peer_manager, "172.16.0.1", 8333);
    printf("✅ %u seed peers added\n", engine->peer_manager->peer_count);

    printf("💰 Initializing test accounts...\n");
    address_t accounts[10];
    for (int i = 0; i < 10; i++)
    {
        memset(&accounts[i], i + 1, ADDRESS_LEN);
        balance_cache_update(engine->balance_cache, &accounts[i], main_token_id, 100000000000000ULL);
        balance_cache_update(engine->balance_cache, &accounts[i], stable_token_id, 1000000000000ULL);
    }
    printf("✅ 10 test accounts created with initial balances\n");

    demonstrate_dex_functionality(engine);
    demonstrate_nft_functionality(engine);
    demonstrate_swap_functionality(engine);
    demonstrate_staking_functionality(engine);
    demonstrate_governance_functionality(engine);

    printf("\n💸 Running initial transaction load...\n");
    for (int i = 0; i < 10000; i++)
    {
        int from = rand() % 10;
        int to = rand() % 10;
        if (from != to)
        {
            transfer_tokens(engine, &accounts[from], &accounts[to], main_token_id, 1000000000, 1, 1);
        }
    }
    printf("✅ 10,000 transactions submitted\n");

    Sleep(3000);
    print_system_stats(engine);

    perform_stress_test(engine, 15);

    Sleep(2000);
    print_system_stats(engine);

    printf("\n🔍 Checking transaction status examples...\n");
    transaction_t *sample_tx = transaction_pool_peek(engine->tx_pool);
    if (sample_tx)
    {
        transaction_status_t status = get_transaction_status(engine, &sample_tx->tx_hash);
        printf("📊 Sample transaction status: %s\n",
               status.status == 0 ? "Pending" : status.status == 1 ? "Confirmed"
                                                                   : "Processing");
        free(sample_tx);
    }

    address_t rich_account = accounts[0];
    amount_t rich_balance = get_balance(engine, &rich_account, main_token_id);
    printf("💰 Rich account balance: %.2f P2P\n", (double)rich_balance / 100000000);

    printf("\n🛑 Shutting down system...\n");
    p2p_engine_stop(engine);

    printf("🧹 Cleaning up resources...\n");
    p2p_engine_destroy(engine);

    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    SYSTEM SHUTDOWN COMPLETE                      ║\n");
    printf("║              Thank you for using P2P Token System                ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

#endif
