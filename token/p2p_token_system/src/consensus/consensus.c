#include "consensus.h"

consensus_t* consensus_create(void) {
    consensus_t* cons = (consensus_t*)calloc(1, sizeof(consensus_t));
    if (!cons) return NULL;

    cons->target_block_time = 10;
    cons->min_difficulty = 0x0000FFFF;
    cons->max_difficulty = 0x00000001;
    cons->adjustment_interval = 2016;

    return cons;
}

void consensus_destroy(consensus_t* cons) {
    free(cons);
}

bool consensus_validate_block(consensus_t* cons, const block_t* block, const blockchain_t* chain) {
    if (!cons || !block || !chain) return false;

    if (block->height != chain->height) return false;

    if (block->height > 0) {
        block_t* prev = blockchain_get_block((blockchain_t*)chain, block->height - 1);
        if (!prev) return false;

        if (memcmp(&block->prev_hash, &prev->block_hash, HASH_LEN) != 0) return false;

        if (block->timestamp <= prev->timestamp) return false;
    }

    uint32_t target = cons->min_difficulty / (block->difficulty ? block->difficulty : 1);
    if (target < 1) target = 1;

    uint32_t hash_prefix = 0;
    memcpy(&hash_prefix, block->block_hash.bytes, 4);

    if (hash_prefix > target) return false;

    return true;
}

uint32_t consensus_calculate_difficulty(consensus_t* cons, const blockchain_t* chain) {
    if (!cons || !chain || chain->height < cons->adjustment_interval) {
        return cons->min_difficulty;
    }

    block_t* first = blockchain_get_block((blockchain_t*)chain, chain->height - cons->adjustment_interval);
    block_t* last = blockchain_get_last_block((blockchain_t*)chain);

    if (!first || !last) return cons->min_difficulty;

    uint32_t actual_time = (uint32_t)(last->timestamp - first->timestamp);
    uint32_t expected_time = cons->adjustment_interval * cons->target_block_time;

    if (actual_time > expected_time * 4) actual_time = expected_time * 4;
    if (actual_time < expected_time / 4) actual_time = expected_time / 4;

    uint32_t new_difficulty = (uint32_t)((uint64_t)last->difficulty * expected_time / actual_time);

    if (new_difficulty < cons->max_difficulty) new_difficulty = cons->max_difficulty;
    if (new_difficulty > cons->min_difficulty) new_difficulty = cons->min_difficulty;

    return new_difficulty;
}

bool consensus_validate_timestamp(consensus_t* cons, const block_t* block, const blockchain_t* chain) {
    (void)cons;

    if (!block || !chain) return false;

    timestamp_t now = time(NULL);
    if (block->timestamp > now + 7200) return false;

    if (block->height > 0) {
        block_t* prev = blockchain_get_block((blockchain_t*)chain, block->height - 1);
        if (prev && block->timestamp <= prev->timestamp) return false;
    }

    return true;
}