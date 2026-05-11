#include "blockchain.h"
#include "../crypto/hash.h"

blockchain_t* blockchain_create(void) {
    blockchain_t* chain = (blockchain_t*)calloc(1, sizeof(blockchain_t));
    if (!chain) return NULL;

    chain->blocks = (block_t**)calloc(MAX_BLOCKS, sizeof(block_t*));
    chain->hashes = (hash_t*)calloc(MAX_BLOCKS, sizeof(hash_t));

    if (!chain->blocks || !chain->hashes) {
        free(chain->blocks);
        free(chain->hashes);
        free(chain);
        return NULL;
    }

    InitializeCriticalSection(&chain->lock);

    block_t* genesis = (block_t*)calloc(1, sizeof(block_t));
    genesis->height = 0;
    genesis->timestamp = time(NULL);
    genesis->difficulty = 0x0000FFFF;
    genesis->reward = 5000000000ULL;
    genesis->version = 1;

    hash_compute((uint8_t*)genesis, sizeof(block_t), &genesis->block_hash);

    chain->blocks[0] = genesis;
    chain->hashes[0] = genesis->block_hash;
    chain->height = 1;
    chain->total_difficulty = genesis->difficulty;
    chain->prune_height = 0;

    return chain;
}

void blockchain_destroy(blockchain_t* chain) {
    if (!chain) return;

    for (block_height_t i = 0; i < chain->height; i++) {
        if (chain->blocks[i]) {
            free(chain->blocks[i]);
        }
    }

    DeleteCriticalSection(&chain->lock);
    free(chain->blocks);
    free(chain->hashes);
    free(chain);
}

bool blockchain_add_block(blockchain_t* chain, block_t* block) {
    if (!chain || !block) return false;

    EnterCriticalSection(&chain->lock);

    if (chain->height >= MAX_BLOCKS) {
        LeaveCriticalSection(&chain->lock);
        return false;
    }

    block->height = chain->height;
    chain->blocks[chain->height] = block;
    chain->hashes[chain->height] = block->block_hash;
    chain->total_difficulty += block->difficulty;
    chain->height++;

    LeaveCriticalSection(&chain->lock);
    return true;
}

block_t* blockchain_get_block(blockchain_t* chain, block_height_t height) {
    if (!chain || height >= chain->height) return NULL;

    EnterCriticalSection(&chain->lock);
    block_t* block = chain->blocks[height];
    LeaveCriticalSection(&chain->lock);
    return block;
}

block_t* blockchain_get_last_block(blockchain_t* chain) {
    if (!chain || chain->height == 0) return NULL;
    return blockchain_get_block(chain, chain->height - 1);
}

bool blockchain_validate(blockchain_t* chain) {
    if (!chain || chain->height == 0) return false;

    EnterCriticalSection(&chain->lock);

    for (block_height_t i = 1; i < chain->height; i++) {
        block_t* block = chain->blocks[i];
        block_t* prev = chain->blocks[i - 1];

        if (!block || !prev) {
            LeaveCriticalSection(&chain->lock);
            return false;
        }

        hash_t computed;
        hash_compute((uint8_t*)block, sizeof(block_t), &computed);

        if (memcmp(&computed, &block->block_hash, HASH_LEN) != 0) {
            LeaveCriticalSection(&chain->lock);
            return false;
        }

        if (memcmp(&block->prev_hash, &prev->block_hash, HASH_LEN) != 0) {
            LeaveCriticalSection(&chain->lock);
            return false;
        }
    }

    LeaveCriticalSection(&chain->lock);
    return true;
}

void blockchain_prune(blockchain_t* chain, uint32_t keep_count) {
    if (!chain || chain->height <= keep_count) return;

    EnterCriticalSection(&chain->lock);

    uint32_t new_prune = chain->height - keep_count;
    if (new_prune > chain->prune_height) {
        for (block_height_t i = chain->prune_height; i < new_prune; i++) {
            if (chain->blocks[i] && i > 0) {
                free(chain->blocks[i]);
                chain->blocks[i] = NULL;
            }
        }
        chain->prune_height = new_prune;
    }

    LeaveCriticalSection(&chain->lock);
}