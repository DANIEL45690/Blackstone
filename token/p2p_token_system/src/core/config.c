#include "config.h"

void p2p_config_default(p2p_config_t* config) {
    if (!config) return;

    config->port = 8333;
    config->max_peers = 100;
    config->worker_threads = 8;
    config->tx_pool_size = 100000;
    config->cache_size = 1000000;
    config->target_block_time_ms = 10000;
    config->min_stake_amount = 1000;
    config->enable_mining = true;
    strcpy(config->data_dir, "./data");
    strcpy(config->network, "mainnet");
}