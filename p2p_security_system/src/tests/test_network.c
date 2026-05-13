#include "test_network.h"
#include "../core/logger.h"
#include "../core/network.h"

int run_network_tests(void)
{
    LOG_INFO("========== NETWORK TESTS ==========");

    network_init();

    int node1 = network_create_node(9001);
    int node2 = network_create_node(9002);

    if (node1 >= 0 && node2 >= 0)
    {
        LOG_INFO("[OK] Nodes created: node1=%d, node2=%d", node1, node2);
    }
    else
    {
        LOG_ERROR("[FAIL] Node creation");
        network_cleanup();
        return 0;
    }

    if (network_connect_node(node2, "127.0.0.1", 9001) == 0)
    {
        LOG_INFO("[OK] Node connection established");
    }
    else
    {
        LOG_ERROR("[FAIL] Node connection");
        network_cleanup();
        return 0;
    }

    if (network_handshake(node2))
    {
        LOG_INFO("[OK] Handshake completed");
    }
    else
    {
        LOG_ERROR("[FAIL] Handshake");
    }

    const char *test_msg = "Hello P2P Network!";
    if (network_send_packet(node2, PACKET_DATA, (const uint8_t *)test_msg, strlen(test_msg) + 1) > 0)
    {
        LOG_INFO("[OK] Packet sent");
    }
    else
    {
        LOG_ERROR("[FAIL] Packet send");
    }

    uint8_t recv_buffer[1024];
    size_t recv_len;
    int pkt_type = network_receive_packet(node1, recv_buffer, sizeof(recv_buffer), &recv_len);

    if (pkt_type == PACKET_DATA && recv_len > 0)
    {
        LOG_INFO("[OK] Packet received: %s", recv_buffer);
    }
    else
    {
        LOG_ERROR("[FAIL] Packet receive");
    }

    network_print_stats();
    network_cleanup();

    LOG_INFO("=================================");
    return 1;
}
