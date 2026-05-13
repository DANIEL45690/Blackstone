#ifndef P2P_CORE_PACKET_H
#define P2P_CORE_PACKET_H

#include "../include/common.h"

typedef struct
{
    uint8_t type;
    uint32_t length;
    uint32_t crc;
    uint16_t timestamp;
} packet_header_t;

typedef struct
{
    packet_header_t header;
    uint8_t *data;
    size_t data_len;
} packet_t;

int packet_create(packet_t *packet, int type, const uint8_t *data, size_t len);
void packet_free(packet_t *packet);
int packet_serialize(const packet_t *packet, uint8_t *buffer, size_t buffer_size);
int packet_deserialize(packet_t *packet, const uint8_t *buffer, size_t buffer_size);
int packet_validate(const packet_t *packet);
int packet_send(socket_t sock, const packet_t *packet);
int packet_receive(socket_t sock, packet_t *packet, uint8_t *data_buffer, size_t buffer_size);

#endif
