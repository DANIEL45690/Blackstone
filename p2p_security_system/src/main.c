/**
 * P2P Security System - Main Entry Point
 * Version: v5.1.2
 */

#include "core/logger.h"
#include "core/crypto.h"
#include "core/network.h"
#include "core/utils.h"
#include "p2p/node.h"
#include "p2p/handshake.h"
#include "p2p/routing.h"
#include "security/firewall.h"
#include "security/auth.h"
#include "security/ratelimit.h"
#include "proxy/proxy_server.h"
#include "tests/test_crypto.h"
#include "tests/test_network.h"
#include "tests/test_p2p.h"

static volatile int g_running = 1;

void signal_handler(int signum)
{
    LOG_INFO("Received signal %d, shutting down...", signum);
    g_running = 0;
}

void print_banner(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                                                                      ║\n");
    printf("║     ██████╗ ██████╗ ███████╗    ███████╗██╗   ██╗███████╗████████╗    ║\n");
    printf("║     ██╔══██╗╚════██╗██╔════╝    ██╔════╝╚██╗ ██╔╝██╔════╝╚══██╔══╝    ║\n");
    printf("║     ██████╔╝ █████╔╝█████╗      ███████╗ ╚████╔╝ ███████╗   ██║       ║\n");
    printf("║     ██╔═══╝  ╚═══██╗██╔══╝      ╚════██║  ╚██╔╝  ╚════██║   ██║       ║\n");
    printf("║     ██║     ██████╔╝███████╗    ███████║   ██║   ███████║   ██║       ║\n");
    printf("║     ╚═╝     ╚═════╝ ╚══════╝    ╚══════╝   ╚═╝   ╚══════╝   ╚═╝       ║\n");
    printf("║                                                                      ║\n");
    printf("║              P2P SECURITY SYSTEM v%d.%d.%d                         ║\n",
           VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
    printf("║              PRODUCTION READY                                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    system("chcp 65001 > nul 2>&1");
    srand((unsigned int)time(NULL));

    print_banner();

    logger_init();

    LOG_INFO("=== P2P SECURITY SYSTEM v%d.%d.%d STARTED ===",
             VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

    if (!crypto_init())
    {
        LOG_FATAL("Failed to initialize crypto provider");
        logger_shutdown();
        return 1;
    }

    network_init();

    firewall_init();
    auth_init();
    ratelimit_init();

    run_crypto_tests();
    run_network_tests();
    run_p2p_tests();

    proxy_server_start(DEFAULT_PROXY_PORT);

    LOG_INFO("System running. Press Ctrl+C to stop.");

    while (g_running)
    {
        Sleep(1000);
        network_check_heartbeats();
        routing_update_tables();
    }

    proxy_server_stop();

    network_cleanup();
    crypto_cleanup();
    firewall_cleanup();
    auth_cleanup();
    ratelimit_cleanup();
    logger_shutdown();

    printf("\n");
    printf("══════════════════════════════════════════════════════════════════════\n");
    printf("  System shutdown complete.\n");
    printf("══════════════════════════════════════════════════════════════════════\n");

    return 0;
}
