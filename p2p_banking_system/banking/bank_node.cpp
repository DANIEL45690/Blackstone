#include "bank_node.h"
#include "../blockchain/transaction.h"
#include <iostream>
#include <iomanip>

HighSecurityBankNode::HighSecurityBankNode(const std::string &id, const std::string &masterKey)
    : logger("bank_" + id + ".log", masterKey), network(id, masterKey), blockchain(logger, id),
      masterCrypto(masterKey), nodeId(id), running(true)
{
    ecc = EllipticCurveCryptography();
    qrs = QuantumResistantSignature(12);
}

HighSecurityBankNode::~HighSecurityBankNode()
{
    shutdown();
}

bool HighSecurityBankNode::initialize()
{
    hotBalances["CentralBank"] = 10000000;
    hotBalances["Reserve"] = 50000000;
    hotBalances["Treasury"] = 25000000;

    logger.info("High Security Bank Node initialized: " + nodeId);
    logger.info("ECC Public Key: " + std::to_string(ecc.getPublic()));
    logger.info("Quantum-Resistant Signature Root: " + qrs.getRoot().substr(0, 16));

    backgroundJobs.emplace_back(&HighSecurityBankNode::processDeposits, this);
    backgroundJobs.emplace_back(&HighSecurityBankNode::auditTransactions, this);
    backgroundJobs.emplace_back(&HighSecurityBankNode::syncLoop, this);
    backgroundJobs.emplace_back(&HighSecurityBankNode::heartbeatLoop, this);
    backgroundJobs.emplace_back(&HighSecurityBankNode::processPendingTransactions, this);

    return true;
}

bool HighSecurityBankNode::startNetwork(int port)
{
    return network.startServer(port);
}

bool HighSecurityBankNode::connectToBank(const std::string &peerId, const std::string &address, int port)
{
    return network.connectToPeer(peerId, address, port);
}

bool HighSecurityBankNode::registerUser(const std::string &userId, const std::string &publicKey, double initialCredit)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    userKeys[userId] = publicKey;
    hotBalances[userId] = initialCredit;
    userCreditLimits[userId] = initialCredit;
    logger.info("User registered: " + userId + " with credit limit: $" + std::to_string(initialCredit));
    return true;
}

bool HighSecurityBankNode::depositToUser(const std::string &userId, double amount)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    if (hotBalances["CentralBank"] >= amount)
    {
        hotBalances["CentralBank"] -= amount;
        hotBalances[userId] += amount;
        logger.info("Deposited $" + std::to_string(amount) + " to " + userId);
        return true;
    }
    logger.warn("Deposit failed for " + userId + ": insufficient central bank funds");
    return false;
}

bool HighSecurityBankNode::validateTransaction(const std::string &from, const std::string &to, double amount, const std::string &signature)
{
    if (from.empty() || to.empty())
        return false;
    if (amount <= 0)
        return false;
    if (hotBalances[from] + userCreditLimits[from] < amount)
        return false;

    std::string txData = from + to + std::to_string(amount) + std::to_string(getTimestampMs());
    std::string expectedSig = sha1_hmac(userKeys[from], txData);

    return expectedSig == signature;
}

bool HighSecurityBankNode::processTransfer(const std::string &from, const std::string &to, double amount, const std::string &signature)
{
    std::lock_guard<std::mutex> lock(bankMutex);

    if (!validateTransaction(from, to, amount, signature))
    {
        logger.error("Invalid transaction from " + from + " to " + to);
        return false;
    }

    hotBalances[from] -= amount;
    hotBalances[to] += amount;
    userHistory[from].push_back("TX:" + to + ":$" + std::to_string(amount));
    userHistory[to].push_back("TX:" + from + ":$" + std::to_string(amount));

    auto tx = BankTransaction::createTransaction(from, to, amount, userKeys[from]);
    std::vector<BankTransaction::Transaction> txs = {tx};
    blockchain.addBlock(txs, nodeId);
    network.broadcast(BankTransaction::serialize(tx));

    logger.info("Transfer processed: " + from + " -> " + to + " $" + std::to_string(amount));
    return true;
}

bool HighSecurityBankNode::withdrawFromUser(const std::string &userId, double amount)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    if (hotBalances[userId] >= amount)
    {
        hotBalances[userId] -= amount;
        hotBalances["CentralBank"] += amount;
        logger.info("Withdrew $" + std::to_string(amount) + " from " + userId);
        return true;
    }
    return false;
}

double HighSecurityBankNode::getUserBalance(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    return hotBalances[userId];
}

std::vector<std::string> HighSecurityBankNode::getUserHistory(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    return userHistory[userId];
}

double HighSecurityBankNode::getUserCreditLimit(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    return userCreditLimits[userId];
}

bool HighSecurityBankNode::setUserCreditLimit(const std::string &userId, double limit)
{
    std::lock_guard<std::mutex> lock(bankMutex);
    userCreditLimits[userId] = limit;
    return true;
}

void HighSecurityBankNode::processDeposits()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(bankMutex);
        for (auto &[user, balance] : hotBalances)
        {
            if (balance > 10000 && user != "CentralBank" && user != "Reserve" && user != "Treasury")
            {
                logger.info("Processing high balance for " + user + ": $" + std::to_string(balance));
            }
        }
    }
}

void HighSecurityBankNode::auditTransactions()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::minutes(10));
        logger.info("AUDIT: Blockchain verification started");
        blockchain.verifyChain();
        blockchain.printBlockchain();
        logger.info("AUDIT: Verification completed");
    }
}

void HighSecurityBankNode::syncLoop()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        network.broadcast("SYNC_REQUEST:" + nodeId);
    }
}

void HighSecurityBankNode::heartbeatLoop()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        network.broadcast("PING:" + nodeId + ":" + std::to_string(getTimestampMs()));
    }
}

void HighSecurityBankNode::processPendingTransactions()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void HighSecurityBankNode::printStatus()
{
    std::lock_guard<std::mutex> lock(bankMutex);
    std::cout << "\n=== HIGH SECURITY BANK STATUS ===\n";
    std::cout << "Node ID: " << nodeId << "\n";
    std::cout << "ECC Public Key: " << ecc.getPublic() << "\n";
    std::cout << "Quantum-Resistant Signatures: Active\n";
    std::cout << "AES-256 Encryption: Active\n";
    std::cout << "Ternary Encoding: Active\n\n";
    std::cout << "Hot Balances:\n";
    for (auto &[user, balance] : hotBalances)
    {
        std::cout << "  " << user << ": $" << std::fixed << std::setprecision(2) << balance << "\n";
    }
    blockchain.printBlockchain();
}

void HighSecurityBankNode::shutdown()
{
    running = false;
    for (auto &t : backgroundJobs)
    {
        if (t.joinable())
            t.join();
    }
    network.shutdown();
    logger.info("Node shutdown complete");
}

bool HighSecurityBankNode::isRunning() const
{
    return running;
}
