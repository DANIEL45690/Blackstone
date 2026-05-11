#ifndef P2P_CONFIG_H
#define P2P_CONFIG_H

#include "../common/types.h"

typedef struct {
    uint16_t port;
    uint32_t max_peers;
    uint32_t worker_threads;
    uint32_t tx_pool_size;
    uint32_t cache_size;
    uint32_t target_block_time_ms;
    uint32_t min_stake_amount;
    bool enable_mining;
    char data_dir[256];
    char network[32];
} p2p_config_t;

void p2p_config_default(p2p_config_t* config);

#endif