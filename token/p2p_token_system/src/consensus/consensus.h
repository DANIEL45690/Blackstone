#ifndef P2P_CONSENSUS_H
#define P2P_CONSENSUS_H

#include "../common/types.h"
#include "../blockchain/blockchain.h"

typedef struct consensus {
    uint32_t target_block_time;
    uint32_t min_difficulty;
    uint32_t max_difficulty;
    uint32_t adjustment_interval;
} consensus_t;

consensus_t* consensus_create(void);
void consensus_destroy(consensus_t* cons);
bool consensus_validate_block(consensus_t* cons, const block_t* block, const blockchain_t* chain);
uint32_t consensus_calculate_difficulty(consensus_t* cons, const blockchain_t* chain);
bool consensus_validate_timestamp(consensus_t* cons, const block_t* block, const blockchain_t* chain);

#endif