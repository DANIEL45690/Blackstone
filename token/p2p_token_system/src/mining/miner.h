#ifndef P2P_MINER_H
#define P2P_MINER_H

#include "../common/types.h"
#include "../transaction/tx_pool.h"
#include "../blockchain/blockchain.h"

typedef struct miner {
    address_t miner_address;
    bool mining_enabled;
    uint32_t max_transactions_per_block;
} miner_t;

miner_t* miner_create(void);
void miner_destroy(miner_t* miner);
void miner_set_address(miner_t* miner, const address_t* addr);
block_t* miner_create_block(miner_t* miner, tx_pool_t* pool, blockchain_t* chain);
bool miner_mine_block(miner_t* miner, block_t* block, uint32_t difficulty);

#endif