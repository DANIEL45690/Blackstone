#include "utils.h"
#include <windows.h>

char *ip_to_string(uint32_t ip, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size < 16)
        return NULL;
    uint8_t *bytes = (uint8_t *)&ip;
    snprintf(buffer, buffer_size, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return buffer;
}

uint32_t string_to_ip(const char *ip_str)
{
    return inet_addr(ip_str);
}

int is_valid_ip(const char *ip_str)
{
    struct in_addr addr;
    return inet_pton(AF_INET, ip_str, &addr) == 1;
}

void print_hex(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (len % 16 != 0)
        printf("\n");
}

void sleep_ms(uint32_t ms)
{
    Sleep(ms);
}

char *get_time_str(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

uint64_t get_uptime_ms(void)
{
    return GetTickCount64();
}
