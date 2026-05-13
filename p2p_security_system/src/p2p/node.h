#ifndef P2P_NODE_H
#define P2P_NODE_H

#include "../include/common.h"

typedef struct p2p_node_t
{
    int id;
    char ip[16];
    int port;
    int connected;
    uint64_t last_seen;
    uint64_t latency_ms;
    uint8_t public_key[256];
} p2p_node_t;

int node_init(void);
void node_cleanup(void);
int node_create(const char *ip, int port);
int node_connect(int node_id, const char *target_ip, int target_port);
int node_disconnect(int node_id);
int node_send(int node_id, const uint8_t *data, size_t len);
int node_broadcast(const uint8_t *data, size_t len);
void node_update_status(void);
void node_print_all(void);

#endif
