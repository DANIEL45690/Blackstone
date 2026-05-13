# P2P Security System API

## Core Modules
- logger_init(), logger_log(), logger_shutdown()
- crypto_init(), random_bytes(), sha256(), crc32()
- network_init(), network_create_node(), network_connect_node()
- network_send_packet(), network_receive_packet(), network_handshake()

## Packet Types
PACKET_HANDSHAKE (0x01), PACKET_DATA (0x02), PACKET_CONTROL (0x03)
PACKET_HEARTBEAT (0x04), PACKET_DISCONNECT (0x05), PACKET_AUTH (0x06)
