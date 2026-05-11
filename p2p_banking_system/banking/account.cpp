#include "account.h"
#include "../core/sha1.h"
#include <sstream>

BankAccount::BankAccount() : balance(0), frozenBalance(0), isFrozen(false), createdAt(0), lastTransactionAt(0) {}

BankAccount::BankAccount(const std::string &id, const std::string &owner, const std::string &pubKey)
    : accountId(id), ownerId(owner), balance(0), frozenBalance(0), accountType("standard"),
      isFrozen(false), createdAt(getTimestampMs()), lastTransactionAt(getTimestampMs()), publicKey(pubKey) {}

std::string BankAccount::getAccountId() const { return accountId; }
std::string BankAccount::getOwnerId() const { return ownerId; }
double BankAccount::getBalance() const { return balance; }
double BankAccount::getAvailableBalance() const { return isFrozen ? frozenBalance : balance; }
std::string BankAccount::getAccountType() const { return accountType; }
bool BankAccount::isAccountFrozen() const { return isFrozen; }
uint64_t BankAccount::getCreatedAt() const { return createdAt; }
uint64_t BankAccount::getLastTransactionAt() const { return lastTransactionAt; }
std::vector<std::string> BankAccount::getTransactionHistory() const { return transactionHistory; }
std::string BankAccount::getPublicKey() const { return publicKey; }

bool BankAccount::deposit(double amount)
{
    if (amount <= 0)
        return false;
    balance += amount;
    lastTransactionAt = getTimestampMs();
    return true;
}

bool BankAccount::withdraw(double amount)
{
    if (amount <= 0)
        return false;
    if (isFrozen && amount > frozenBalance)
        return false;
    if (!isFrozen && amount > balance)
        return false;
    balance -= amount;
    lastTransactionAt = getTimestampMs();
    return true;
}

bool BankAccount::freeze()
{
    if (isFrozen)
        return false;
    isFrozen = true;
    frozenBalance = balance;
    return true;
}

bool BankAccount::unfreeze()
{
    if (!isFrozen)
        return false;
    isFrozen = false;
    frozenBalance = 0;
    return true;
}

bool BankAccount::setAccountType(const std::string &type)
{
    accountType = type;
    return true;
}

void BankAccount::addTransaction(const std::string &txId)
{
    transactionHistory.push_back(txId);
    lastTransactionAt = getTimestampMs();
}

void BankAccount::setEncryptedPrivateKey(const std::string &encrypted)
{
    encryptedPrivateKey = encrypted;
}

std::string BankAccount::getEncryptedPrivateKey() const
{
    return encryptedPrivateKey;
}

std::string BankAccount::serialize() const
{
    std::stringstream ss;
    ss << accountId << "|" << ownerId << "|" << balance << "|" << frozenBalance << "|"
       << accountType << "|" << isFrozen << "|" << createdAt << "|" << lastTransactionAt << "|"
       << publicKey << "|" << encryptedPrivateKey;
    for (const auto &tx : transactionHistory)
    {
        ss << "|" << tx;
    }
    return ss.str();
}

BankAccount BankAccount::deserialize(const std::string &data)
{
    BankAccount acc;
    std::stringstream ss(data);
    std::getline(ss, acc.accountId, '|');
    std::getline(ss, acc.ownerId, '|');
    std::string balStr;
    std::getline(ss, balStr, '|');
    acc.balance = std::stod(balStr);
    std::string fbalStr;
    std::getline(ss, fbalStr, '|');
    acc.frozenBalance = std::stod(fbalStr);
    std::getline(ss, acc.accountType, '|');
    std::string frozenStr;
    std::getline(ss, frozenStr, '|');
    acc.isFrozen = frozenStr == "1";
    std::string createdStr;
    std::getline(ss, createdStr, '|');
    acc.createdAt = std::stoull(createdStr);
    std::string lastStr;
    std::getline(ss, lastStr, '|');
    acc.lastTransactionAt = std::stoull(lastStr);
    std::getline(ss, acc.publicKey, '|');
    std::getline(ss, acc.encryptedPrivateKey, '|');
    std::string tx;
    while (std::getline(ss, tx, '|'))
    {
        if (!tx.empty())
            acc.transactionHistory.push_back(tx);
    }
    return acc;
}

bool AccountManager::createAccount(const std::string &accountId, const std::string &ownerId, const std::string &publicKey)
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    if (accounts.find(accountId) != accounts.end())
        return false;
    BankAccount acc(accountId, ownerId, publicKey);
    accounts[accountId] = acc;
    ownerToAccounts[ownerId].push_back(accountId);
    return true;
}

bool AccountManager::deleteAccount(const std::string &accountId)
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    auto it = accounts.find(accountId);
    if (it == accounts.end())
        return false;
    std::string owner = it->second.getOwnerId();
    accounts.erase(it);
    auto &vec = ownerToAccounts[owner];
    vec.erase(std::remove(vec.begin(), vec.end(), accountId), vec.end());
    return true;
}

BankAccount AccountManager::getAccount(const std::string &accountId) const
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    auto it = accounts.find(accountId);
    if (it != accounts.end())
        return it->second;
    return BankAccount();
}

std::vector<BankAccount> AccountManager::getAccountsByOwner(const std::string &ownerId) const
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    std::vector<BankAccount> result;
    auto it = ownerToAccounts.find(ownerId);
    if (it != ownerToAccounts.end())
    {
        for (const auto &accId : it->second)
        {
            result.push_back(accounts.at(accId));
        }
    }
    return result;
}

bool AccountManager::updateBalance(const std::string &accountId, double amount)
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    auto it = accounts.find(accountId);
    if (it == accounts.end())
        return false;
    if (amount > 0)
        return it->second.deposit(amount);
    if (amount < 0)
        return it->second.withdraw(-amount);
    return true;
}

bool AccountManager::freezeAccount(const std::string &accountId)
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    auto it = accounts.find(accountId);
    if (it == accounts.end())
        return false;
    return it->second.freeze();
}

bool AccountManager::unfreezeAccount(const std::string &accountId)
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    auto it = accounts.find(accountId);
    if (it == accounts.end())
        return false;
    return it->second.unfreeze();
}

size_t AccountManager::getAccountCount() const
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    return accounts.size();
}

bool AccountManager::accountExists(const std::string &accountId) const
{
    std::lock_guard<std::mutex> lock(accountsMutex);
    return accounts.find(accountId) != accounts.end();
}
