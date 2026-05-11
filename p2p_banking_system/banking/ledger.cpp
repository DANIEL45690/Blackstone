#include "ledger.h"
#include "../core/sha1.h"
#include "../core/random.h"

LedgerEntry::LedgerEntry() : amount(0), timestamp(0), balanceAfter(0) {}

LedgerEntry::LedgerEntry(const std::string &accId, double amt, const std::string &txId, const std::string &type)
    : accountId(accId), amount(amt), transactionId(txId), entryType(type), timestamp(getTimestampMs()), balanceAfter(0)
{
    UltraFastRandom rng;
    entryId = sha1(accId + std::to_string(timestamp) + rng.bytes(16));
}

std::string LedgerEntry::serialize() const
{
    std::stringstream ss;
    ss << entryId << "|" << accountId << "|" << amount << "|" << transactionId << "|"
       << timestamp << "|" << entryType << "|" << balanceAfter;
    return ss.str();
}

LedgerEntry LedgerEntry::deserialize(const std::string &data)
{
    LedgerEntry entry;
    std::stringstream ss(data);
    std::getline(ss, entry.entryId, '|');
    std::getline(ss, entry.accountId, '|');
    std::string amtStr;
    std::getline(ss, amtStr, '|');
    entry.amount = std::stod(amtStr);
    std::getline(ss, entry.transactionId, '|');
    std::string tsStr;
    std::getline(ss, tsStr, '|');
    entry.timestamp = std::stoull(tsStr);
    std::getline(ss, entry.entryType, '|');
    std::string balStr;
    std::getline(ss, balStr, '|');
    entry.balanceAfter = std::stod(balStr);
    return entry;
}

Ledger::Ledger(size_t max) : maxEntries(max) {}

bool Ledger::addEntry(const LedgerEntry &entry)
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    if (entries.size() >= maxEntries)
    {
        entries.erase(entries.begin());
    }
    entries.push_back(entry);
    balances[entry.accountId] = entry.balanceAfter;
    accountEntries[entry.accountId].push_back(entry.entryId);
    return true;
}

bool Ledger::addTransaction(const std::string &accountId, double amount, const std::string &transactionId, const std::string &type)
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    double currentBalance = balances[accountId];
    double newBalance = (type == "credit") ? currentBalance + amount : currentBalance - amount;

    LedgerEntry entry(accountId, amount, transactionId, type);
    entry.balanceAfter = newBalance;

    entries.push_back(entry);
    balances[accountId] = newBalance;
    accountEntries[accountId].push_back(entry.entryId);

    if (entries.size() > maxEntries)
    {
        entries.erase(entries.begin());
    }
    return true;
}

double Ledger::getBalance(const std::string &accountId) const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    auto it = balances.find(accountId);
    if (it != balances.end())
        return it->second;
    return 0;
}

std::vector<LedgerEntry> Ledger::getEntriesForAccount(const std::string &accountId, size_t limit) const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    std::vector<LedgerEntry> result;
    auto it = accountEntries.find(accountId);
    if (it != accountEntries.end())
    {
        for (const auto &entryId : it->second)
        {
            for (const auto &entry : entries)
            {
                if (entry.entryId == entryId)
                {
                    result.push_back(entry);
                    break;
                }
            }
        }
    }
    if (result.size() > limit)
        result.resize(limit);
    return result;
}

std::vector<LedgerEntry> Ledger::getEntriesSince(uint64_t timestamp) const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    std::vector<LedgerEntry> result;
    for (const auto &entry : entries)
    {
        if (entry.timestamp >= timestamp)
        {
            result.push_back(entry);
        }
    }
    return result;
}

bool Ledger::verifyConsistency() const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    std::map<std::string, double> computedBalances;
    for (const auto &entry : entries)
    {
        if (entry.entryType == "credit")
        {
            computedBalances[entry.accountId] += entry.amount;
        }
        else
        {
            computedBalances[entry.accountId] -= entry.amount;
        }
        if (computedBalances[entry.accountId] != entry.balanceAfter)
            return false;
    }
    return true;
}

size_t Ledger::getEntryCount() const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    return entries.size();
}

void Ledger::clear()
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    entries.clear();
    balances.clear();
    accountEntries.clear();
}

std::string Ledger::serialize() const
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    std::stringstream ss;
    for (const auto &entry : entries)
    {
        ss << entry.serialize() << ";";
    }
    return ss.str();
}

void Ledger::deserialize(const std::string &data)
{
    std::lock_guard<std::mutex> lock(ledgerMutex);
    clear();
    std::stringstream ss(data);
    std::string entryStr;
    while (std::getline(ss, entryStr, ';'))
    {
        if (!entryStr.empty())
        {
            LedgerEntry entry = LedgerEntry::deserialize(entryStr);
            entries.push_back(entry);
            balances[entry.accountId] = entry.balanceAfter;
            accountEntries[entry.accountId].push_back(entry.entryId);
        }
    }
}
