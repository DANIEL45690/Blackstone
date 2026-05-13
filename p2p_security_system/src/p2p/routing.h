#ifndef P2P_ROUTING_H
#define P2P_ROUTING_H

#include "../include/common.h"

typedef struct route_entry_t
{
    uint32_t dest_ip;
    int dest_port;
    int next_hop_id;
    int metric;
    uint64_t last_update;
    int valid;
} route_entry_t;

int routing_init(void);
void routing_cleanup(void);
int routing_add_route(uint32_t dest_ip, int dest_port, int next_hop, int metric);
int routing_remove_route(uint32_t dest_ip, int dest_port);
route_entry_t *routing_find_route(uint32_t dest_ip, int dest_port);
void routing_print_table(void);
void routing_update_tables(void);
int routing_get_next_hop(uint32_t dest_ip, int dest_port);

#endif
