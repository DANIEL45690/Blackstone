#ifndef P2P_CORE_H
#define P2P_CORE_H

#include "../common/types.h"
#include "../common/config.h"
#include "../blockchain/blockchain.h"
#include "../transaction/tx_pool.h"
#include "../token/token_registry.h"
#include "../balance/balance_cache.h"
#include "../network/peer_manager.h"
#include "../consensus/consensus.h"
#include "../mining/miner.h"

typedef struct p2p_core {
    blockchain_t* blockchain;
    tx_pool_t* tx_pool;
    token_registry_t* token_registry;
    balance_cache_t* balance_cache;
    peer_manager_t* peer_manager;
    consensus_t* consensus;
    miner_t* miner;

    HANDLE worker_threads[32];
    HANDLE network_thread;
    HANDLE consensus_thread;
    HANDLE cleanup_thread;
    uint32_t worker_count;

    volatile bool running;
    volatile uint64_t total_transactions;
    volatile uint64_t total_blocks;
    timestamp_t start_time;

    CRITICAL_SECTION stats_lock;
    p2p_config_t config;
} p2p_core_t;

p2p_core_t* p2p_core_create(const p2p_config_t* config);
void p2p_core_destroy(p2p_core_t* core);
bool p2p_core_start(p2p_core_t* core);
void p2p_core_stop(p2p_core_t* core);
bool p2p_core_is_running(p2p_core_t* core);
void p2p_core_get_stats(p2p_core_t* core, p2p_stats_t* stats);
bool p2p_core_submit_transaction(p2p_core_t* core, const transaction_t* tx);
bool p2p_core_transfer(p2p_core_t* core, const address_t* from, const address_t* to, token_id_t token_id, amount_t amount);

#endif