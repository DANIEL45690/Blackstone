#include "transaction.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <sstream>
#include <chrono>

BankTransaction::Transaction::Transaction() : amount(0), timestamp(0), nonce(0) {}

BankTransaction::Transaction::Transaction(const std::string &from, const std::string &to, double amt)
    : fromAccount(from), toAccount(to), amount(amt), timestamp(0), nonce(0) {}

std::string BankTransaction::generateTxId(const std::string &from, const std::string &to, double amount, uint64_t timestamp, uint64_t nonce)
{
    return sha1(from + to + std::to_string(amount) + std::to_string(timestamp) + std::to_string(nonce));
}

BankTransaction::Transaction BankTransaction::createTransaction(const std::string &from, const std::string &to, double amount, const std::string &privateKey)
{
    Transaction tx;
    UltraFastRandom rng;
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now().time_since_epoch())
                             .count();
    uint64_t nonce = rng.u64();
    tx.txId = generateTxId(from, to, amount, timestamp, nonce);
    tx.fromAccount = from;
    tx.toAccount = to;
    tx.amount = amount;
    tx.timestamp = timestamp;
    tx.nonce = nonce;
    std::string data = tx.txId + tx.fromAccount + tx.toAccount + std::to_string(tx.amount) +
                       std::to_string(tx.timestamp) + std::to_string(tx.nonce);
    tx.signature = sha1_hmac(privateKey, data);
    tx.hash = calculateHash(tx);
    return tx;
}

bool BankTransaction::verifyTransaction(const Transaction &tx, const std::string &publicKey)
{
    std::string data = tx.txId + tx.fromAccount + tx.toAccount + std::to_string(tx.amount) +
                       std::to_string(tx.timestamp) + std::to_string(tx.nonce);
    std::string expectedSig = sha1_hmac(publicKey, data);
    return expectedSig == tx.signature;
}

std::string BankTransaction::serialize(const Transaction &tx)
{
    std::stringstream ss;
    ss << tx.txId << "|" << tx.fromAccount << "|" << tx.toAccount << "|" << tx.amount << "|"
       << tx.timestamp << "|" << tx.nonce << "|" << tx.signature << "|" << tx.hash << "|" << tx.metadata;
    return ss.str();
}

BankTransaction::Transaction BankTransaction::deserialize(const std::string &data)
{
    Transaction tx;
    std::stringstream ss(data);
    std::getline(ss, tx.txId, '|');
    std::getline(ss, tx.fromAccount, '|');
    std::getline(ss, tx.toAccount, '|');
    std::string amountStr;
    std::getline(ss, amountStr, '|');
    tx.amount = std::stod(amountStr);
    std::string tsStr;
    std::getline(ss, tsStr, '|');
    tx.timestamp = std::stoull(tsStr);
    std::string nonceStr;
    std::getline(ss, nonceStr, '|');
    tx.nonce = std::stoull(nonceStr);
    std::getline(ss, tx.signature, '|');
    std::getline(ss, tx.hash, '|');
    std::getline(ss, tx.metadata, '|');
    return tx;
}

std::string BankTransaction::calculateHash(const Transaction &tx)
{
    std::string data = tx.txId + tx.fromAccount + tx.toAccount + std::to_string(tx.amount) +
                       std::to_string(tx.timestamp) + std::to_string(tx.nonce) + tx.signature;
    return sha1_double(data);
}

std::string BankTransaction::calculateMerkleRoot(std::vector<Transaction> &transactions)
{
    if (transactions.empty())
        return sha1("empty");
    std::vector<std::string> hashes;
    for (auto &tx : transactions)
        hashes.push_back(tx.hash);
    while (hashes.size() > 1)
    {
        if (hashes.size() % 2 == 1)
            hashes.push_back(hashes.back());
        std::vector<std::string> newHashes;
        for (size_t i = 0; i < hashes.size(); i += 2)
        {
            newHashes.push_back(sha1(hashes[i] + hashes[i + 1]));
        }
        hashes = newHashes;
    }
    return hashes[0];
}

bool BankTransaction::validateTransaction(const Transaction &tx, double senderBalance)
{
    if (tx.amount <= 0)
        return false;
    if (tx.amount > senderBalance)
        return false;
    if (tx.fromAccount.empty() || tx.toAccount.empty())
        return false;
    if (tx.fromAccount == tx.toAccount)
        return false;
    return true;
}
