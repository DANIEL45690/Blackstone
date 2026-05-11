#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <vector>
#include <cstdint>

class BankAccount
{
    std::string accountId;
    std::string ownerId;
    double balance;
    double frozenBalance;
    std::string accountType;
    bool isFrozen;
    uint64_t createdAt;
    uint64_t lastTransactionAt;
    std::vector<std::string> transactionHistory;
    std::string publicKey;
    std::string encryptedPrivateKey;

public:
    BankAccount();
    BankAccount(const std::string &id, const std::string &owner, const std::string &pubKey);

    std::string getAccountId() const;
    std::string getOwnerId() const;
    double getBalance() const;
    double getAvailableBalance() const;
    std::string getAccountType() const;
    bool isAccountFrozen() const;
    uint64_t getCreatedAt() const;
    uint64_t getLastTransactionAt() const;
    std::vector<std::string> getTransactionHistory() const;
    std::string getPublicKey() const;

    bool deposit(double amount);
    bool withdraw(double amount);
    bool freeze();
    bool unfreeze();
    bool setAccountType(const std::string &type);
    void addTransaction(const std::string &txId);
    void setEncryptedPrivateKey(const std::string &encrypted);
    std::string getEncryptedPrivateKey() const;

    std::string serialize() const;
    static BankAccount deserialize(const std::string &data);
};

class AccountManager
{
    std::map<std::string, BankAccount> accounts;
    std::map<std::string, std::vector<std::string>> ownerToAccounts;
    std::mutex accountsMutex;

public:
    bool createAccount(const std::string &accountId, const std::string &ownerId, const std::string &publicKey);
    bool deleteAccount(const std::string &accountId);
    BankAccount getAccount(const std::string &accountId) const;
    std::vector<BankAccount> getAccountsByOwner(const std::string &ownerId) const;
    bool updateBalance(const std::string &accountId, double amount);
    bool freezeAccount(const std::string &accountId);
    bool unfreezeAccount(const std::string &accountId);
    size_t getAccountCount() const;
    bool accountExists(const std::string &accountId) const;
};

#endif
