#ifndef BLOCK_H
#define BLOCK_H

#include "transaction.h"
#include <string>
#include <vector>
#include <cstdint>

class Block
{
public:
    int index;
    std::string previousHash;
    uint64_t timestamp;
    std::vector<BankTransaction::Transaction> transactions;
    std::string merkleRoot;
    std::string hash;
    uint64_t nonce;
    std::string validator;
    uint64_t difficulty;
    std::string version;

    Block();
    Block(int idx, const std::string &prevHash);

    std::string calculateHash() const;
    bool mineBlock(uint64_t targetDifficulty, uint64_t maxAttempts);
    bool validateBlock(const Block &previousBlock) const;
    std::string serialize() const;
    static Block deserialize(const std::string &data);

private:
    std::string getHeaderData() const;
};

class BlockHeader
{
public:
    int version;
    std::string previousBlockHash;
    std::string merkleRootHash;
    uint64_t timestamp;
    uint64_t difficultyTarget;
    uint64_t nonce;

    std::string hash() const;
    std::string serialize() const;
    static BlockHeader deserialize(const std::string &data);
};

#endif
