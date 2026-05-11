#include "blockchain.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <iostream>
#include <algorithm>

BankingBlockchain::BankingBlockchain()
{
    chain.push_back(createGenesisBlock());
}

Block BankingBlockchain::createGenesisBlock()
{
    Block genesis(0, sha1("GENESIS"));
    genesis.timestamp = 0;
    genesis.validator = "SYSTEM";
    genesis.merkleRoot = sha1("GENESIS_BLOCK");
    genesis.nonce = 0;
    genesis.hash = genesis.calculateHash();
    return genesis;
}

bool BankingBlockchain::validateTransactions(const std::vector<BankTransaction::Transaction> &transactions)
{
    std::map<std::string, double> tempBalances = balances;
    for (const auto &tx : transactions)
    {
        if (!BankTransaction::verifyTransaction(tx, tx.fromAccount))
            return false;
        if (tempBalances[tx.fromAccount] < tx.amount)
            return false;
        tempBalances[tx.fromAccount] -= tx.amount;
        tempBalances[tx.toAccount] += tx.amount;
    }
    return true;
}

void BankingBlockchain::updateBalances(const std::vector<BankTransaction::Transaction> &transactions)
{
    for (const auto &tx : transactions)
    {
        balances[tx.fromAccount] -= tx.amount;
        balances[tx.toAccount] += tx.amount;
        accountTransactions[tx.fromAccount].push_back(tx.txId);
        accountTransactions[tx.toAccount].push_back(tx.txId);
    }
}

bool BankingBlockchain::addBlock(std::vector<BankTransaction::Transaction> &transactions, const std::string &validator)
{
    std::lock_guard<std::mutex> lock(blockchainMutex);

    if (!validateTransactions(transactions))
        return false;

    Block newBlock;
    newBlock.index = chain.size();
    newBlock.previousHash = chain.back().hash;
    newBlock.timestamp = getTimestampMs();
    newBlock.transactions = transactions;
    newBlock.merkleRoot = BankTransaction::calculateMerkleRoot(transactions);
    newBlock.validator = validator;
    newBlock.difficulty = 4;

    if (!newBlock.mineBlock(4, 1000000))
        return false;

    updateBalances(transactions);
    chain.push_back(newBlock);
    blockIndex[newBlock.hash] = newBlock;

    return true;
}

bool BankingBlockchain::addBlockDirect(const Block &block)
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    if (block.index != (int)chain.size())
        return false;
    if (block.previousHash != chain.back().hash)
        return false;
    if (block.calculateHash() != block.hash)
        return false;

    if (!validateTransactions(block.transactions))
        return false;

    updateBalances(block.transactions);
    chain.push_back(block);
    blockIndex[block.hash] = block;
    return true;
}

Block BankingBlockchain::getBlock(int index) const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    if (index < 0 || index >= (int)chain.size())
        return Block();
    return chain[index];
}

Block BankingBlockchain::getLatestBlock() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    if (chain.empty())
        return Block();
    return chain.back();
}

std::string BankingBlockchain::getLatestHash() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    if (chain.empty())
        return "";
    return chain.back().hash;
}

int BankingBlockchain::getHeight() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    return chain.size();
}

double BankingBlockchain::getBalance(const std::string &account)
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    return balances[account];
}

std::vector<std::string> BankingBlockchain::getAccountTransactions(const std::string &account) const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    auto it = accountTransactions.find(account);
    if (it == accountTransactions.end())
        return {};
    return it->second;
}

bool BankingBlockchain::verifyChain() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    for (size_t i = 1; i < chain.size(); i++)
    {
        if (chain[i].previousHash != chain[i - 1].hash)
            return false;
        if (chain[i].calculateHash() != chain[i].hash)
            return false;
        if (chain[i].index != (int)i)
            return false;
        std::string computedMerkle = BankTransaction::calculateMerkleRoot(
            const_cast<std::vector<BankTransaction::Transaction> &>(chain[i].transactions));
        if (chain[i].merkleRoot != computedMerkle)
            return false;
    }
    return true;
}

void BankingBlockchain::printBlockchain() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    std::cout << "\n=== BLOCKCHAIN STATUS ===\n";
    std::cout << "Height: " << chain.size() << "\n";
    for (const auto &block : chain)
    {
        std::cout << "Block " << block.index << " | Hash: " << block.hash.substr(0, 16)
                  << " | Tx: " << block.transactions.size() << " | Validator: " << block.validator << "\n";
    }
}

bool BankingBlockchain::containsBlock(const std::string &hash) const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    return blockIndex.find(hash) != blockIndex.end();
}

Block BankingBlockchain::findBlockByHash(const std::string &hash) const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    auto it = blockIndex.find(hash);
    if (it != blockIndex.end())
        return it->second;
    return Block();
}

std::vector<Block> BankingBlockchain::getBlocksFrom(int startIndex) const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    if (startIndex < 0 || startIndex >= (int)chain.size())
        return {};
    return std::vector<Block>(chain.begin() + startIndex, chain.end());
}

double BankingBlockchain::getTotalSupply() const
{
    std::lock_guard<std::mutex> lock(blockchainMutex);
    double total = 0;
    for (const auto &[account, balance] : balances)
    {
        total += balance;
    }
    return total;
}

bool BlockchainValidator::validateBlockStructure(const Block &block)
{
    if (block.index < 0)
        return false;
    if (block.previousHash.empty())
        return false;
    if (block.timestamp == 0)
        return false;
    if (block.merkleRoot.empty())
        return false;
    if (block.hash.empty())
        return false;
    if (block.validator.empty())
        return false;
    return true;
}

bool BlockchainValidator::validateTransactionOrder(const std::vector<BankTransaction::Transaction> &txs)
{
    for (size_t i = 1; i < txs.size(); i++)
    {
        if (txs[i].timestamp < txs[i - 1].timestamp)
            return false;
    }
    return true;
}

bool BlockchainValidator::validateMerkleRoot(const Block &block)
{
    std::vector<BankTransaction::Transaction> txs = block.transactions;
    std::string computedRoot = BankTransaction::calculateMerkleRoot(txs);
    return computedRoot == block.merkleRoot;
}

bool BlockchainValidator::validateTimestamp(const Block &block, uint64_t maxFutureMs)
{
    uint64_t now = getTimestampMs();
    if (block.timestamp > now + maxFutureMs)
        return false;
    return true;
}

int BlockchainValidator::calculateNextDifficulty(const std::vector<Block> &recentBlocks, int targetBlockTimeMs)
{
    if (recentBlocks.size() < 2)
        return 4;
    uint64_t totalTime = recentBlocks.back().timestamp - recentBlocks.front().timestamp;
    int actualBlockTime = totalTime / (recentBlocks.size() - 1);
    if (actualBlockTime < targetBlockTimeMs / 2)
        return 5;
    if (actualBlockTime > targetBlockTimeMs * 2)
        return 3;
    return 4;
}
