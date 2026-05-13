#ifndef P2P_COMMON_H
#define P2P_COMMON_H

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
typedef SOCKET socket_t;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#define close_socket(s) closesocket(s)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET_VAL -1
#define close_socket(s) close(s)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <stdint.h>

/* Version */
#define VERSION_MAJOR 5
#define VERSION_MINOR 1
#define VERSION_PATCH 2

/* System limits */
#define MAX_THREADS 64
#define MAX_NODES 16
#define MAX_BUFFER_SIZE 65536

/* Network defaults */
#define DEFAULT_PROXY_PORT 8888
#define DEFAULT_CONTROL_PORT 9999

/* Protocol constants */
#define HEARTBEAT_INTERVAL_MS 5000
#define CONNECTION_TIMEOUT_MS 30000

/* Crypto */
#define AES_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define SHA256_DIGEST_SIZE 32
#define MAX_PACKET_SIZE 65536

/* Log levels */
typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} log_level_t;

/* Packet types */
typedef enum
{
    PACKET_HANDSHAKE = 0x01,
    PACKET_DATA = 0x02,
    PACKET_CONTROL = 0x03,
    PACKET_HEARTBEAT = 0x04,
    PACKET_DISCONNECT = 0x05,
    PACKET_AUTH = 0x06
} packet_type_t;

/* Hash type */
typedef struct
{
    uint8_t bytes[SHA256_DIGEST_SIZE];
} hash_t;

#endif /* P2P_COMMON_H */
