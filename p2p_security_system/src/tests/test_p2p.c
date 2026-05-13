#include "test_p2p.h"
#include "../core/logger.h"
#include "../p2p/node.h"
#include "../p2p/handshake.h"
#include "../p2p/routing.h"
#include "../security/firewall.h"
#include "../security/auth.h"
#include "../security/ratelimit.h"

int run_p2p_tests(void)
{
    LOG_INFO("========== P2P TESTS ==========");

    node_init();
    handshake_init();
    routing_init();
    firewall_init();
    auth_init();
    ratelimit_init();

    LOG_INFO("[OK] All P2P subsystems initialized");

    int node1 = node_create("0.0.0.0", 8001);
    int node2 = node_create("0.0.0.0", 8002);

    if (node1 >= 0 && node2 >= 0)
    {
        LOG_INFO("[OK] P2P nodes created: %d, %d", node1, node2);
    }
    else
    {
        LOG_ERROR("[FAIL] P2P node creation");
    }

    routing_add_route(inet_addr("192.168.1.1"), 8080, node1, 1);
    routing_add_route(inet_addr("10.0.0.1"), 9090, node2, 2);
    routing_print_table();
    LOG_INFO("[OK] Routing table test passed");

    firewall_add_rule("192.168.1.100", 22, FW_ACTION_DENY, 100);
    firewall_add_rule("10.0.0.1", 443, FW_ACTION_ALLOW, 50);
    firewall_print_rules();
    LOG_INFO("[OK] Firewall test passed");

    auth_add_user("testuser", "testpass123", 0);
    if (auth_verify("testuser", "testpass123"))
    {
        LOG_INFO("[OK] Authentication test passed");
    }
    else
    {
        LOG_ERROR("[FAIL] Authentication test");
    }
    auth_print_users();

    if (ratelimit_check("192.168.1.1", 10, 60000))
    {
        LOG_INFO("[OK] Rate limit test passed");
    }

    node_cleanup();
    handshake_cleanup();
    routing_cleanup();
    firewall_cleanup();
    auth_cleanup();
    ratelimit_cleanup();

    LOG_INFO("=================================");
    return 1;
}
