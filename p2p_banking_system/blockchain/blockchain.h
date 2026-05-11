#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "block.h"
#include <vector>
#include <map>
#include <mutex>
#include <functional>

class BankingBlockchain
{
    std::vector<Block> chain;
    std::map<std::string, double> balances;
    std::map<std::string, std::vector<std::string>> accountTransactions;
    std::map<std::string, Block> blockIndex;
    std::mutex blockchainMutex;

    Block createGenesisBlock();
    bool validateTransactions(const std::vector<BankTransaction::Transaction> &transactions);
    void updateBalances(const std::vector<BankTransaction::Transaction> &transactions);

public:
    BankingBlockchain();

    bool addBlock(std::vector<BankTransaction::Transaction> &transactions, const std::string &validator);
    bool addBlockDirect(const Block &block);
    Block getBlock(int index) const;
    Block getLatestBlock() const;
    std::string getLatestHash() const;
    int getHeight() const;
    double getBalance(const std::string &account);
    std::vector<std::string> getAccountTransactions(const std::string &account) const;
    bool verifyChain() const;
    void printBlockchain() const;
    bool containsBlock(const std::string &hash) const;
    Block findBlockByHash(const std::string &hash) const;
    std::vector<Block> getBlocksFrom(int startIndex) const;
    double getTotalSupply() const;
};

class BlockchainValidator
{
public:
    static bool validateBlockStructure(const Block &block);
    static bool validateTransactionOrder(const std::vector<BankTransaction::Transaction> &txs);
    static bool validateMerkleRoot(const Block &block);
    static bool validateTimestamp(const Block &block, uint64_t maxFutureMs);
    static int calculateNextDifficulty(const std::vector<Block> &recentBlocks, int targetBlockTimeMs);
};

#endif
