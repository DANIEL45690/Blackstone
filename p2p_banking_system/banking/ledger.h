#ifndef LEDGER_H
#define LEDGER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>

class LedgerEntry
{
public:
    std::string entryId;
    std::string accountId;
    double amount;
    std::string transactionId;
    uint64_t timestamp;
    std::string entryType;
    double balanceAfter;

    LedgerEntry();
    LedgerEntry(const std::string &accId, double amt, const std::string &txId, const std::string &type);

    std::string serialize() const;
    static LedgerEntry deserialize(const std::string &data);
};

class Ledger
{
    std::vector<LedgerEntry> entries;
    std::map<std::string, double> balances;
    std::map<std::string, std::vector<std::string>> accountEntries;
    std::mutex ledgerMutex;
    size_t maxEntries;

public:
    explicit Ledger(size_t max = 1000000);

    bool addEntry(const LedgerEntry &entry);
    bool addTransaction(const std::string &accountId, double amount, const std::string &transactionId, const std::string &type);
    double getBalance(const std::string &accountId) const;
    std::vector<LedgerEntry> getEntriesForAccount(const std::string &accountId, size_t limit = 100) const;
    std::vector<LedgerEntry> getEntriesSince(uint64_t timestamp) const;
    bool verifyConsistency() const;
    size_t getEntryCount() const;
    void clear();

    std::string serialize() const;
    void deserialize(const std::string &data);
};

#endif
