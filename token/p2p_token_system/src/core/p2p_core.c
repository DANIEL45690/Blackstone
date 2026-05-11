#include "p2p_core.h"
#include <process.h>

static DWORD WINAPI worker_thread_func(LPVOID arg) {
    p2p_core_t* core = (p2p_core_t*)arg;

    while (core->running) {
        transaction_t* tx = tx_pool_pop(core->tx_pool);
        if (tx) {
            if (tx_verify(tx, core->balance_cache, core->blockchain->height)) {
                amount_t fee = tx->amount / 1000;
                if (fee < 1) fee = 1;
                amount_t total = tx->amount + fee;

                balance_cache_update(core->balance_cache, &tx->from, tx->token_id, -total);
                balance_cache_update(core->balance_cache, &tx->to, tx->token_id, tx->amount);

                InterlockedIncrement64((LONG64*)&core->total_transactions);
            }
            free(tx);
        } else {
            Sleep(1);
        }
    }
    return 0;
}

static DWORD WINAPI network_thread_func(LPVOID arg) {
    p2p_core_t* core = (p2p_core_t*)arg;

    while (core->running) {
        peer_manager_update(core->peer_manager);
        Sleep(100);
    }
    return 0;
}

static DWORD WINAPI consensus_thread_func(LPVOID arg) {
    p2p_core_t* core = (p2p_core_t*)arg;

    while (core->running) {
        if (core->tx_pool->size >= 5 || core->blockchain->height < 5) {
            block_t* block = miner_create_block(core->miner, core->tx_pool, core->blockchain);
            if (block) {
                if (consensus_validate_block(core->consensus, block, core->blockchain)) {
                    blockchain_add_block(core->blockchain, block);

                    balance_cache_update(core->balance_cache, &block->miner, 0, block->reward);
                    InterlockedIncrement64((LONG64*)&core->total_blocks);

                    for (uint32_t i = 0; i < block->tx_count; i++) {
                        if (block->transactions[i]) {
                            free(block->transactions[i]);
                        }
                    }
                }
                free(block);
            }
        }
        Sleep(1000);
    }
    return 0;
}

static DWORD WINAPI cleanup_thread_func(LPVOID arg) {
    p2p_core_t* core = (p2p_core_t*)arg;

    while (core->running) {
        Sleep(60000);
        blockchain_prune(core->blockchain, 1000);
        tx_pool_cleanup(core->tx_pool);
    }
    return 0;
}

p2p_core_t* p2p_core_create(const p2p_config_t* config) {
    p2p_core_t* core = (p2p_core_t*)calloc(1, sizeof(p2p_core_t));
    if (!core) return NULL;

    core->config = *config;
    core->running = false;
    core->total_transactions = 0;
    core->total_blocks = 0;
    core->start_time = time(NULL);
    core->worker_count = config->worker_threads;
    if (core->worker_count > 32) core->worker_count = 32;

    InitializeCriticalSection(&core->stats_lock);

    core->blockchain = blockchain_create();
    core->tx_pool = tx_pool_create(config->tx_pool_size);
    core->token_registry = token_registry_create();
    core->balance_cache = balance_cache_create(config->cache_size);
    core->peer_manager = peer_manager_create(config->max_peers);
    core->consensus = consensus_create();
    core->miner = miner_create();

    if (!core->blockchain || !core->tx_pool || !core->token_registry ||
        !core->balance_cache || !core->peer_manager || !core->consensus || !core->miner) {
        p2p_core_destroy(core);
        return NULL;
    }

    address_t genesis_addr = {0};
    genesis_addr.bytes[0] = 0xFF;
    token_t main_token = token_create("P2P", "P2P Token", &genesis_addr, 10000000000ULL, 18);
    token_registry_register(core->token_registry, &main_token);

    return core;
}

void p2p_core_destroy(p2p_core_t* core) {
    if (!core) return;

    if (core->blockchain) blockchain_destroy(core->blockchain);
    if (core->tx_pool) tx_pool_destroy(core->tx_pool);
    if (core->token_registry) token_registry_destroy(core->token_registry);
    if (core->balance_cache) balance_cache_destroy(core->balance_cache);
    if (core->peer_manager) peer_manager_destroy(core->peer_manager);
    if (core->consensus) consensus_destroy(core->consensus);
    if (core->miner) miner_destroy(core->miner);

    DeleteCriticalSection(&core->stats_lock);
    free(core);
}

bool p2p_core_start(p2p_core_t* core) {
    if (!core) return false;

    core->running = true;

    for (uint32_t i = 0; i < core->worker_count; i++) {
        core->worker_threads[i] = CreateThread(NULL, 0, worker_thread_func, core, 0, NULL);
        if (core->worker_threads[i]) {
            SetThreadPriority(core->worker_threads[i], THREAD_PRIORITY_HIGHEST);
        }
    }

    core->network_thread = CreateThread(NULL, 0, network_thread_func, core, 0, NULL);
    core->consensus_thread = CreateThread(NULL, 0, consensus_thread_func, core, 0, NULL);
    core->cleanup_thread = CreateThread(NULL, 0, cleanup_thread_func, core, 0, NULL);

    return true;
}

void p2p_core_stop(p2p_core_t* core) {
    if (!core) return;

    core->running = false;

    for (uint32_t i = 0; i < core->worker_count; i++) {
        if (core->worker_threads[i]) {
            WaitForSingleObject(core->worker_threads[i], 3000);
            CloseHandle(core->worker_threads[i]);
        }
    }

    if (core->network_thread) {
        WaitForSingleObject(core->network_thread, 3000);
        CloseHandle(core->network_thread);
    }
    if (core->consensus_thread) {
        WaitForSingleObject(core->consensus_thread, 3000);
        CloseHandle(core->consensus_thread);
    }
    if (core->cleanup_thread) {
        WaitForSingleObject(core->cleanup_thread, 3000);
        CloseHandle(core->cleanup_thread);
    }
}

bool p2p_core_is_running(p2p_core_t* core) {
    return core ? core->running : false;
}

void p2p_core_get_stats(p2p_core_t* core, p2p_stats_t* stats) {
    if (!core || !stats) return;

    EnterCriticalSection(&core->stats_lock);
    stats->block_height = core->blockchain->height;
    stats->pool_size = core->tx_pool->size;
    stats->active_peers = core->peer_manager->active_count;
    stats->total_transactions = core->total_transactions;
    stats->total_blocks = core->total_blocks;
    stats->cache_hit_rate = balance_cache_get_hit_rate(core->balance_cache);
    stats->uptime = (uint64_t)difftime(time(NULL), core->start_time);
    stats->total_tokens = core->token_registry->count;
    stats->cache_entries = core->balance_cache->count;
    LeaveCriticalSection(&core->stats_lock);
}

bool p2p_core_submit_transaction(p2p_core_t* core, const transaction_t* tx) {
    if (!core || !tx) return false;
    return tx_pool_add(core->tx_pool, tx);
}

bool p2p_core_transfer(p2p_core_t* core, const address_t* from, const address_t* to, token_id_t token_id, amount_t amount) {
    if (!core || !from || !to || amount == 0) return false;

    transaction_t tx;
    memset(&tx, 0, sizeof(tx));
    tx.from = *from;
    tx.to = *to;
    tx.amount = amount;
    tx.token_id = token_id;
    tx.timestamp = time(NULL);
    tx.nonce = (uint64_t)GetTickCount();
    tx.fee = amount / 1000;
    if (tx.fee < 1) tx.fee = 1;
    tx.gas_price = 1;
    tx.gas_limit = 21000;
    tx.version = 1;

    tx_compute_hash(&tx);
    return tx_pool_add(core->tx_pool, &tx);
}