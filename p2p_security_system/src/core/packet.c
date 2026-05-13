#include "packet.h"
#include "crypto.h"
#include "utils.h"
#include <stdlib.h>

int packet_create(packet_t *packet, int type, const uint8_t *data, size_t len)
{
    if (!packet || (data == NULL && len > 0))
        return -1;

    packet->header.type = (uint8_t)type;
    packet->header.length = htonl((uint32_t)len);
    packet->header.crc = htonl(crc32(data, len));
    packet->header.timestamp = htons((uint16_t)(get_timestamp_ms() & 0xFFFF));

    if (len > 0)
    {
        packet->data = (uint8_t *)malloc(len);
        if (!packet->data)
            return -1;
        memcpy(packet->data, data, len);
    }
    else
    {
        packet->data = NULL;
    }
    packet->data_len = len;

    return 0;
}

void packet_free(packet_t *packet)
{
    if (packet && packet->data)
    {
        secure_zero(packet->data, packet->data_len);
        free(packet->data);
        packet->data = NULL;
        packet->data_len = 0;
    }
}

int packet_serialize(const packet_t *packet, uint8_t *buffer, size_t buffer_size)
{
    if (!packet || !buffer || buffer_size < 12 + packet->data_len)
        return -1;

    buffer[0] = packet->header.type;
    *(uint32_t *)(buffer + 1) = packet->header.length;
    *(uint32_t *)(buffer + 5) = packet->header.crc;
    *(uint16_t *)(buffer + 9) = packet->header.timestamp;

    if (packet->data_len > 0 && packet->data)
    {
        memcpy(buffer + 12, packet->data, packet->data_len);
    }

    return 12 + (int)packet->data_len;
}

int packet_deserialize(packet_t *packet, const uint8_t *buffer, size_t buffer_size)
{
    if (!packet || !buffer || buffer_size < 12)
        return -1;

    packet->header.type = buffer[0];
    packet->header.length = *(uint32_t *)(buffer + 1);
    packet->header.crc = *(uint32_t *)(buffer + 5);
    packet->header.timestamp = *(uint16_t *)(buffer + 9);

    uint32_t data_len = ntohl(packet->header.length);

    if (buffer_size < 12 + data_len)
        return -1;

    if (data_len > 0)
    {
        packet->data = (uint8_t *)malloc(data_len);
        if (!packet->data)
            return -1;
        memcpy(packet->data, buffer + 12, data_len);
        packet->data_len = data_len;
    }
    else
    {
        packet->data = NULL;
        packet->data_len = 0;
    }

    if (!packet_validate(packet))
    {
        packet_free(packet);
        return -1;
    }

    return 0;
}

int packet_validate(const packet_t *packet)
{
    if (!packet)
        return 0;

    uint32_t expected_crc = crc32(packet->data, packet->data_len);
    uint32_t actual_crc = ntohl(packet->header.crc);

    return expected_crc == actual_crc;
}

int packet_send(socket_t sock, const packet_t *packet)
{
    uint8_t header[12];
    header[0] = packet->header.type;
    *(uint32_t *)(header + 1) = packet->header.length;
    *(uint32_t *)(header + 5) = packet->header.crc;
    *(uint16_t *)(header + 9) = packet->header.timestamp;

    int sent = send(sock, (char *)header, 12, 0);
    if (sent != 12)
        return -1;

    if (packet->data_len > 0)
    {
        sent = send(sock, (char *)packet->data, (int)packet->data_len, 0);
        if (sent != (int)packet->data_len)
            return -1;
    }

    return 0;
}

int packet_receive(socket_t sock, packet_t *packet, uint8_t *data_buffer, size_t buffer_size)
{
    uint8_t header[12];
    int received = recv(sock, (char *)header, 12, MSG_PEEK);
    if (received < 12)
        return 0;

    received = recv(sock, (char *)header, 12, 0);
    if (received != 12)
        return -1;

    uint32_t data_len = ntohl(*(uint32_t *)(header + 1));

    if (data_len > buffer_size)
        return -1;

    received = recv(sock, (char *)data_buffer, data_len, 0);
    if (received != (int)data_len)
        return -1;

    packet->header.type = header[0];
    packet->header.length = *(uint32_t *)(header + 1);
    packet->header.crc = *(uint32_t *)(header + 5);
    packet->header.timestamp = *(uint16_t *)(header + 9);
    packet->data = data_buffer;
    packet->data_len = data_len;

    if (!packet_validate(packet))
        return -1;

    return packet->header.type;
}
