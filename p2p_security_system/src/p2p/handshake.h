#ifndef P2P_HANDSHAKE_H
#define P2P_HANDSHAKE_H

#include "../include/common.h"

typedef struct
{
    uint8_t protocol_version;
    uint32_t timestamp;
    uint8_t random[32];
    uint8_t public_key[256];
    uint8_t signature[64];
} handshake_request_t;

typedef struct
{
    uint8_t random[32];
    uint8_t session_key_encrypted[256];
    uint8_t signature[64];
} handshake_response_t;

int handshake_init(void);
void handshake_cleanup(void);
int handshake_perform(int node_id);
int handshake_verify_request(const handshake_request_t *req);
int handshake_verify_response(const handshake_response_t *resp);
void handshake_generate_session_key(uint8_t *output, size_t len);

#endif
