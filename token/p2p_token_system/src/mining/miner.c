#include "miner.h"
#include "../crypto/hash.h"

miner_t* miner_create(void) {
    miner_t* miner = (miner_t*)calloc(1, sizeof(miner_t));
    if (!miner) return NULL;

    miner->mining_enabled = true;
    miner->max_transactions_per_block = 100;
    memset(&miner->miner_address, 0, ADDRESS_LEN);

    return miner;
}

void miner_destroy(miner_t* miner) {
    free(miner);
}

void miner_set_address(miner_t* miner, const address_t* addr) {
    if (miner && addr) {
        miner->miner_address = *addr;
    }
}

static hash_t compute_merkle_root(transaction_t** txs, uint32_t count) {
    if (count == 0) {
        hash_t empty = {0};
        return empty;
    }

    if (count == 1) {
        return txs[0]->tx_hash;
    }

    uint32_t level_size = count;
    hash_t* levels = (hash_t*)alloca(count * sizeof(hash_t));

    for (uint32_t i = 0; i < count; i++) {
        levels[i] = txs[i]->tx_hash;
    }

    while (level_size > 1) {
        for (uint32_t i = 0; i < level_size / 2; i++) {
            uint8_t combined[HASH_LEN * 2];
            memcpy(combined, &levels[i * 2], HASH_LEN);
            memcpy(combined + HASH_LEN, &levels[i * 2 + 1], HASH_LEN);
            hash_compute(combined, HASH_LEN * 2, &levels[i]);
        }

        if (level_size & 1) {
            levels[level_size / 2] = levels[level_size - 1];
            level_size = level_size / 2 + 1;
        } else {
            level_size /= 2;
        }
    }

    return levels[0];
}

block_t* miner_create_block(miner_t* miner, tx_pool_t* pool, blockchain_t* chain) {
    if (!miner || !pool || !chain) return NULL;

    block_t* block = (block_t*)calloc(1, sizeof(block_t));
    if (!block) return NULL;

    block->timestamp = time(NULL);
    block->height = chain->height;
    block->miner = miner->miner_address;
    block->reward = 5000000000ULL;
    block->difficulty = 0x0000FFFF;
    block->version = 1;

    if (chain->height > 0) {
        block_t* last = blockchain_get_last_block(chain);
        if (last) {
            block->prev_hash = last->block_hash;
        }
    }

    transaction_t* txs[100];
    uint32_t tx_count = 0;

    for (uint32_t i = 0; i < miner->max_transactions_per_block && tx_count < 100; i++) {
        transaction_t* tx = tx_pool_pop(pool);
        if (tx) {
            txs[tx_count++] = tx;
        } else {
            break;
        }
    }

    for (uint32_t i = 0; i < tx_count; i++) {
        block->transactions[i] = txs[i];
    }
    block->tx_count = tx_count;

    if (tx_count > 0) {
        block->merkle_root = compute_merkle_root(block->transactions, tx_count);
    }

    return block;
}

bool miner_mine_block(miner_t* miner, block_t* block, uint32_t difficulty) {
    if (!miner || !block) return false;
    (void)difficulty;

    uint32_t target = 0x0000FFFF / (block->difficulty ? block->difficulty : 1);
    if (target < 1) target = 1;

    for (uint32_t nonce = 0; nonce < 0xFFFFFFFF; nonce++) {
        block->nonce = nonce;
        hash_compute((uint8_t*)block, sizeof(block_t), &block->block_hash);

        uint32_t hash_prefix = 0;
        memcpy(&hash_prefix, block->block_hash.bytes, 4);

        if (hash_prefix <= target) {
            return true;
        }
    }

    return false;
}