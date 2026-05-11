#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <winmm.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winmm.lib")
typedef SOCKET socket_t;
#define INVALID_SOCKET_T INVALID_SOCKET
#define SOCKET_ERROR_T SOCKET_ERROR
#define close_socket closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#include <signal.h>
typedef int socket_t;
#define INVALID_SOCKET_T -1
#define SOCKET_ERROR_T -1
#define close_socket close
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

uint64_t getTimestampMs();
uint64_t getTimestampUs();
std::string getCurrentTimeISO();
void sleepMs(uint64_t ms);
std::string getHostname();
std::string getIPAddress();

#endif
