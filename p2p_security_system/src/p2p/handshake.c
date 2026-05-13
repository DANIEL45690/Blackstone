#include "handshake.h"
#include "../core/logger.h"
#include "../core/crypto.h"
#include "../core/network.h"

static int g_initialized = 0;

int handshake_init(void)
{
    if (g_initialized)
        return 1;
    g_initialized = 1;
    LOG_INFO("Handshake subsystem initialized");
    return 1;
}

void handshake_cleanup(void)
{
    if (!g_initialized)
        return;
    g_initialized = 0;
    LOG_INFO("Handshake subsystem cleaned up");
}

int handshake_perform(int node_id)
{
    return network_handshake(node_id);
}

int handshake_verify_request(const handshake_request_t *req)
{
    if (!req)
        return 0;
    (void)req;
    return 1;
}

int handshake_verify_response(const handshake_response_t *resp)
{
    if (!resp)
        return 0;
    (void)resp;
    return 1;
}

void handshake_generate_session_key(uint8_t *output, size_t len)
{
    random_bytes(output, len);
}
