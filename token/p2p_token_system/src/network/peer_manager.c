#include "peer_manager.h"

peer_manager_t* peer_manager_create(uint32_t max_peers) {
    peer_manager_t* pm = (peer_manager_t*)calloc(1, sizeof(peer_manager_t));
    if (!pm) return NULL;

    pm->peers = (peer_t*)calloc(max_peers, sizeof(peer_t));
    if (!pm->peers) {
        free(pm);
        return NULL;
    }

    pm->max_peers = max_peers;
    pm->count = 0;
    pm->active_count = 0;
    InitializeCriticalSection(&pm->lock);

    return pm;
}

void peer_manager_destroy(peer_manager_t* pm) {
    if (!pm) return;

    DeleteCriticalSection(&pm->lock);
    free(pm->peers);
    free(pm);
}

bool peer_manager_add(peer_manager_t* pm, const char* ip, uint16_t port) {
    if (!pm || !ip || pm->count >= pm->max_peers) return false;

    EnterCriticalSection(&pm->lock);

    for (uint32_t i = 0; i < pm->count; i++) {
        if (strcmp(pm->peers[i].ip, ip) == 0 && pm->peers[i].port == port) {
            LeaveCriticalSection(&pm->lock);
            return true;
        }
    }

    peer_t* peer = &pm->peers[pm->count];
    peer->id = pm->count;
    strncpy(peer->ip, ip, 15);
    peer->port = port;
    peer->last_seen = time(NULL);
    peer->latency = 0;
    peer->active = true;
    peer->reputation = 100;
    peer->fail_count = 0;

    pm->count++;
    pm->active_count++;

    LeaveCriticalSection(&pm->lock);
    return true;
}

bool peer_manager_remove(peer_manager_t* pm, peer_id_t id) {
    if (!pm || id >= pm->count) return false;

    EnterCriticalSection(&pm->lock);

    if (pm->peers[id].active) {
        pm->active_count--;
    }

    for (uint32_t i = id; i < pm->count - 1; i++) {
        pm->peers[i] = pm->peers[i + 1];
        pm->peers[i].id = i;
    }

    pm->count--;

    LeaveCriticalSection(&pm->lock);
    return true;
}

peer_t* peer_manager_get(peer_manager_t* pm, peer_id_t id) {
    if (!pm || id >= pm->count) return NULL;
    return &pm->peers[id];
}

void peer_manager_update(peer_manager_t* pm) {
    if (!pm) return;

    EnterCriticalSection(&pm->lock);

    timestamp_t now = time(NULL);

    for (uint32_t i = 0; i < pm->count; i++) {
        if (pm->peers[i].active) {
            pm->peers[i].latency = (GetTickCount() % 100) + 1;

            if (now - pm->peers[i].last_seen > 300) {
                pm->peers[i].active = false;
                pm->active_count--;
            }
        }
    }

    LeaveCriticalSection(&pm->lock);
}

uint32_t peer_manager_get_active_count(peer_manager_t* pm) {
    if (!pm) return 0;

    EnterCriticalSection(&pm->lock);
    uint32_t count = pm->active_count;
    LeaveCriticalSection(&pm->lock);

    return count;
}