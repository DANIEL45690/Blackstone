#ifndef P2P_CORE_UTILS_H
#define P2P_CORE_UTILS_H

#include "../include/common.h"

char *ip_to_string(uint32_t ip, char *buffer, size_t buffer_size);
uint32_t string_to_ip(const char *ip_str);
int is_valid_ip(const char *ip_str);
void print_hex(const uint8_t *data, size_t len);
void sleep_ms(uint32_t ms);
char *get_time_str(char *buffer, size_t buffer_size);
uint64_t get_uptime_ms(void);

#endif
