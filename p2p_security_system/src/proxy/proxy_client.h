#ifndef P2P_PROXY_CLIENT_H
#define P2P_PROXY_CLIENT_H

#include "../include/common.h"

int proxy_client_connect(const char *proxy_ip, int proxy_port, const char *target_ip, int target_port);
int proxy_client_disconnect(int client_id);
int proxy_client_send(int client_id, const uint8_t *data, size_t len);
int proxy_client_receive(int client_id, uint8_t *buffer, size_t buffer_size);
void proxy_client_print_all(void);

#endif
