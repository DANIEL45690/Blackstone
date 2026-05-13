#ifndef P2P_CORE_NETWORK_H
#define P2P_CORE_NETWORK_H

#include "../include/common.h"

typedef struct node_t
{
    socket_t socket;
    struct sockaddr_in addr;
    char ip[16];
    int port;
    int connected;
    int authenticated;
    uint8_t session_key[AES_KEY_SIZE];
    volatile uint64_t packets_sent;
    volatile uint64_t packets_received;
    volatile uint64_t bytes_sent;
    volatile uint64_t bytes_received;
    volatile uint64_t errors;
    volatile uint64_t last_heartbeat;
    volatile uint64_t last_activity;
    volatile uint64_t connection_start;
    CRITICAL_SECTION lock;
} node_t;

void network_init(void);
void network_cleanup(void);
int network_create_node(int port);
int network_connect_node(int node_id, const char *target_ip, int target_port);
int network_send_packet(int node_id, int type, const uint8_t *data, size_t len);
int network_receive_packet(int node_id, uint8_t *buffer, size_t buffer_size, size_t *out_len);
int network_handshake(int node_id);
void network_print_stats(void);
void network_check_heartbeats(void);
int network_get_node_count(void);
node_t *network_get_node(int node_id);
uint64_t get_timestamp_ms(void);

#endif
