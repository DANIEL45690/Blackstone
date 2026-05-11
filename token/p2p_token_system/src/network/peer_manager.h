#ifndef P2P_PEER_MANAGER_H
#define P2P_PEER_MANAGER_H

#include "../common/types.h"

typedef struct peer {
    peer_id_t id;
    char ip[16];
    uint16_t port;
    timestamp_t last_seen;
    uint32_t latency;
    bool active;
    uint32_t reputation;
    uint32_t fail_count;
} peer_t;

typedef struct peer_manager {
    peer_t* peers;
    uint32_t count;
    uint32_t max_peers;
    uint32_t active_count;
    CRITICAL_SECTION lock;
} peer_manager_t;

peer_manager_t* peer_manager_create(uint32_t max_peers);
void peer_manager_destroy(peer_manager_t* pm);
bool peer_manager_add(peer_manager_t* pm, const char* ip, uint16_t port);
bool peer_manager_remove(peer_manager_t* pm, peer_id_t id);
peer_t* peer_manager_get(peer_manager_t* pm, peer_id_t id);
void peer_manager_update(peer_manager_t* pm);
uint32_t peer_manager_get_active_count(peer_manager_t* pm);

#endif