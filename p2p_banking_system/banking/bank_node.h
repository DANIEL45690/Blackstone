#ifndef BANK_NODE_H
#define BANK_NODE_H

#include "../core/platform.h"
#include "../core/aes256.h"
#include "../core/ecc.h"
#include "../core/quantum_signature.h"
#include "../network/p2p_network.h"
#include "../blockchain/blockchain.h"
#include "../security/secure_logger.h"
#include <map>
#include <thread>
#include <atomic>

class HighSecurityBankNode
{
    SecureLogger logger;
    P2PNetworkCore network;
    BankingBlockchain blockchain;
    AdvancedAES256 masterCrypto;
    EllipticCurveCryptography ecc;
    QuantumResistantSignature qrs;
    std::string nodeId;
    std::map<std::string, double> hotBalances;
    std::map<std::string, std::string> userKeys;
    std::map<std::string, std::vector<std::string>> userHistory;
    std::map<std::string, double> userCreditLimits;
    std::mutex bankMutex;
    std::vector<std::thread> backgroundJobs;
    std::atomic<bool> running;

    void processDeposits();
    void auditTransactions();
    void syncLoop();
    void heartbeatLoop();
    void processPendingTransactions();

    bool validateTransaction(const std::string &from, const std::string &to, double amount, const std::string &signature);

public:
    HighSecurityBankNode(const std::string &id, const std::string &masterKey);
    ~HighSecurityBankNode();

    bool initialize();
    bool startNetwork(int port);
    bool connectToBank(const std::string &peerId, const std::string &address, int port);

    bool registerUser(const std::string &userId, const std::string &publicKey, double initialCredit = 0);
    bool depositToUser(const std::string &userId, double amount);
    bool processTransfer(const std::string &from, const std::string &to, double amount, const std::string &signature);
    bool withdrawFromUser(const std::string &userId, double amount);

    double getUserBalance(const std::string &userId);
    std::vector<std::string> getUserHistory(const std::string &userId);
    double getUserCreditLimit(const std::string &userId);
    bool setUserCreditLimit(const std::string &userId, double limit);

    void printStatus();
    void shutdown();
    bool isRunning() const;
};

#endif
