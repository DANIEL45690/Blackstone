#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <cstdint>

class BankTransaction
{
public:
    struct Transaction
    {
        std::string txId;
        std::string fromAccount;
        std::string toAccount;
        double amount;
        uint64_t timestamp;
        uint64_t nonce;
        std::string signature;
        std::string hash;
        std::string metadata;

        Transaction();
        Transaction(const std::string &from, const std::string &to, double amt);
    };

    static Transaction createTransaction(const std::string &from, const std::string &to, double amount, const std::string &privateKey);
    static bool verifyTransaction(const Transaction &tx, const std::string &publicKey);
    static std::string serialize(const Transaction &tx);
    static Transaction deserialize(const std::string &data);
    static std::string calculateHash(const Transaction &tx);
    static std::string calculateMerkleRoot(std::vector<Transaction> &transactions);
    static bool validateTransaction(const Transaction &tx, double senderBalance);

private:
    static std::string generateTxId(const std::string &from, const std::string &to, double amount, uint64_t timestamp, uint64_t nonce);
};

#endif
