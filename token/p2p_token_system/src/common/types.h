#ifndef P2P_TYPES_H
#define P2P_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

#define TOKEN_SYMBOL_LEN 16
#define TOKEN_NAME_LEN 64
#define ADDRESS_LEN 32
#define HASH_LEN 32
#define TX_ID_LEN 64
#define MAX_TOKENS 10000
#define MAX_TRANSACTIONS 1000000
#define MAX_PEERS 256
#define MAX_BLOCKS 100000
#define BLOCK_MAX_TX 1000
#define CACHE_LINE_SIZE 64

typedef uint64_t timestamp_t;
typedef uint64_t amount_t;
typedef uint64_t nonce_t;
typedef uint32_t token_id_t;
typedef uint32_t peer_id_t;
typedef uint32_t block_height_t;

typedef struct {
    uint8_t bytes[HASH_LEN];
} hash_t;

typedef struct {
    uint8_t bytes[ADDRESS_LEN];
} address_t;

typedef struct {
    char symbol[TOKEN_SYMBOL_LEN];
    char name[TOKEN_NAME_LEN];
    token_id_t id;
    address_t issuer;
    amount_t total_supply;
    amount_t circulating_supply;
    uint8_t decimals;
    timestamp_t created_at;
    uint32_t holders;
    bool frozen;
    bool mintable;
} token_t;

typedef struct {
    address_t from;
    address_t to;
    amount_t amount;
    token_id_t token_id;
    timestamp_t timestamp;
    nonce_t nonce;
    hash_t tx_hash;
    hash_t signature;
    uint32_t fee;
    uint32_t gas_price;
    uint64_t gas_limit;
    uint8_t version;
    bool processed;
} transaction_t;

typedef struct {
    hash_t block_hash;
    hash_t prev_hash;
    timestamp_t timestamp;
    block_height_t height;
    uint32_t tx_count;
    uint32_t nonce;
    hash_t merkle_root;
    address_t miner;
    amount_t reward;
    uint32_t difficulty;
    transaction_t* transactions[BLOCK_MAX_TX];
} block_t;

typedef struct {
    uint32_t block_height;
    uint64_t pool_size;
    uint32_t active_peers;
    uint64_t total_transactions;
    uint64_t total_blocks;
    double cache_hit_rate;
    uint64_t uptime;
    uint32_t total_tokens;
    uint64_t cache_entries;
} p2p_stats_t;

static inline uint64_t rdtsc(void) {
    return __rdtsc();
}

#endif