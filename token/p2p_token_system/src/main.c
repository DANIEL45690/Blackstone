#include "core/p2p_core.h"
#include "core/config.h"
#include <signal.h>
#include <stdio.h>

volatile int g_running = 1;

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\n[INFO] Shutdown signal received\n");
}

void print_banner(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    P2P TOKEN SYSTEM v4.0                               ║\n");
    printf("║                    High Performance P2P Network                       ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    signal(SIGINT, signal_handler);
    SetConsoleOutputCP(CP_UTF8);
    print_banner();

    p2p_config_t config;
    p2p_config_default(&config);

    printf("[INIT] Loading configuration...\n");
    printf("[INIT] Network port: %d\n", config.port);
    printf("[INIT] Max peers: %d\n", config.max_peers);
    printf("[INIT] Worker threads: %d\n", config.worker_threads);

    p2p_core_t* core = p2p_core_create(&config);
    if (!core) {
        printf("[ERROR] Failed to create core\n");
        return 1;
    }

    printf("[INIT] Starting P2P engine...\n");
    if (!p2p_core_start(core)) {
        printf("[ERROR] Failed to start engine\n");
        p2p_core_destroy(core);
        return 1;
    }

    printf("[READY] System is running\n");
    printf("[INFO] Press Ctrl+C to stop\n\n");

    uint64_t last_tx = 0;
    while (g_running && p2p_core_is_running(core)) {
        Sleep(5000);

        p2p_stats_t stats;
        p2p_core_get_stats(core, &stats);

        uint64_t tps = (stats.total_transactions - last_tx) / 5;
        last_tx = stats.total_transactions;

        printf("\n╔════════════════════════════════════════════════════════════════════════╗\n");
        printf("║                         SYSTEM STATISTICS                              ║\n");
        printf("╠════════════════════════════════════════════════════════════════════════╣\n");
        printf("║ Blockchain Height:    %-12u                                    ║\n", stats.block_height);
        printf("║ Transactions/sec:     %-12llu                                    ║\n", (unsigned long long)tps);
        printf("║ Pool Size:            %-12llu                                    ║\n", (unsigned long long)stats.pool_size);
        printf("║ Active Peers:         %-12u                                    ║\n", stats.active_peers);
        printf("║ Total TX Processed:   %-12llu                                    ║\n", (unsigned long long)stats.total_transactions);
        printf("║ Total Blocks:         %-12llu                                    ║\n", (unsigned long long)stats.total_blocks);
        printf("║ Cache Hit Rate:       %-12.2f%%                                    ║\n", stats.cache_hit_rate);
        printf("║ Uptime:               %-12llu sec                                ║\n", (unsigned long long)stats.uptime);
        printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    }

    printf("\n[SHUTDOWN] Stopping system...\n");
    p2p_core_stop(core);
    p2p_core_destroy(core);

    printf("[DONE] Shutdown complete\n");
    return 0;
}