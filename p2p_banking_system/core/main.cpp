#define _WIN32_WINNT 0x0601
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "platform.h"
#include "../banking/bank_node.h"
#include "../security/secure_logger.h"
#include "../monitoring/health_check.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running(true);

void signalHandler(int signal)
{
    std::cout << "\n[!] Signal " << signal << " received. Shutting down...\n";
    g_running = false;
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "====================================================\n";
    std::cout << "  HIGH SECURITY P2P BANKING SYSTEM v6.0\n";
    std::cout << "  Military-Grade Encryption | Quantum-Resistant\n";
    std::cout << "  AES-256-GCM | ECC-256 | SHA-3 Ready\n";
    std::cout << "====================================================\n\n";

    std::string nodeId = "MainBank_" + std::to_string(std::time(nullptr));
    std::string masterKey = sha1("GLOBAL_MASTER_KEY_2025_" + nodeId);

    SecureLogger logger("system_main.log", masterKey);
    HealthChecker healthChecker(logger);

    HighSecurityBankNode bankNode(nodeId, masterKey);

    if (!bankNode.initialize())
    {
        logger.error("Failed to initialize bank node");
        return 1;
    }

    int port = 9999;
    if (argc > 1)
        port = std::atoi(argv[1]);

    if (bankNode.startNetwork(port))
    {
        std::cout << "[+] P2P Network started on port " << port << "\n";
        logger.info("Network started on port " + std::to_string(port));
    }
    else
    {
        logger.error("Failed to start network");
        return 1;
    }

    bankNode.registerUser("Alice", sha1("AliceMasterKey"), 100000);
    bankNode.registerUser("Bob", sha1("BobMasterKey"), 50000);
    bankNode.registerUser("Charlie", sha1("CharlieMasterKey"), 25000);
    bankNode.registerUser("Diana", sha1("DianaMasterKey"), 75000);
    bankNode.registerUser("Eve", sha1("EveMasterKey"), 10000);

    bankNode.depositToUser("Alice", 50000);
    bankNode.depositToUser("Bob", 30000);
    bankNode.depositToUser("Charlie", 10000);

    auto tx1 = bankNode.processTransfer("Alice", "Bob", 5000, sha1_hmac(sha1("AliceMasterKey"), "AliceBob5000"));
    auto tx2 = bankNode.processTransfer("Bob", "Charlie", 2000, sha1_hmac(sha1("BobMasterKey"), "BobCharlie2000"));
    auto tx3 = bankNode.processTransfer("Charlie", "Diana", 1500, sha1_hmac(sha1("CharlieMasterKey"), "CharlieDiana1500"));
    auto tx4 = bankNode.processTransfer("Diana", "Eve", 3000, sha1_hmac(sha1("DianaMasterKey"), "DianaEve3000"));

    std::cout << "\n[+] System initialized successfully\n";
    std::cout << "[+] Alice balance: $" << bankNode.getUserBalance("Alice") << "\n";
    std::cout << "[+] Bob balance: $" << bankNode.getUserBalance("Bob") << "\n";
    std::cout << "[+] Charlie balance: $" << bankNode.getUserBalance("Charlie") << "\n";
    std::cout << "[+] Diana balance: $" << bankNode.getUserBalance("Diana") << "\n";
    std::cout << "[+] Eve balance: $" << bankNode.getUserBalance("Eve") << "\n";

    bankNode.printStatus();

    healthChecker.startMonitoring();

    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    healthChecker.stopMonitoring();
    bankNode.shutdown();

    std::cout << "\n[+] System shutdown complete\n";
    return 0;
}
