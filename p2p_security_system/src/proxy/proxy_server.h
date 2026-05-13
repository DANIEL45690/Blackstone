#ifndef P2P_PROXY_SERVER_H
#define P2P_PROXY_SERVER_H

#include "../include/common.h"

typedef struct proxy_client_t
{
    socket_t client_socket;
    socket_t target_socket;
    char client_ip[16];
    int client_port;
    uint64_t connected_at;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    int active;
} proxy_client_t;

int proxy_server_start(int port);
void proxy_server_stop(void);
int proxy_server_status(void);
void proxy_server_print_stats(void);
int proxy_forward_data(socket_t from_sock, socket_t to_sock, uint8_t *buffer, size_t buffer_size);

#endif
