#ifndef P2P_BLOCKCHAIN_H
#define P2P_BLOCKCHAIN_H

#include "../common/types.h"

typedef struct blockchain {
    block_t** blocks;
    block_height_t height;
    hash_t* hashes;
    CRITICAL_SECTION lock;
    uint64_t total_difficulty;
    uint32_t prune_height;
} blockchain_t;

blockchain_t* blockchain_create(void);
void blockchain_destroy(blockchain_t* chain);
bool blockchain_add_block(blockchain_t* chain, block_t* block);
block_t* blockchain_get_block(blockchain_t* chain, block_height_t height);
block_t* blockchain_get_last_block(blockchain_t* chain);
bool blockchain_validate(blockchain_t* chain);
void blockchain_prune(blockchain_t* chain, uint32_t keep_count);

#endif