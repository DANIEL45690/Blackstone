#ifndef P2P_SECURITY_RATELIMIT_H
#define P2P_SECURITY_RATELIMIT_H

#include "../include/common.h"

typedef struct
{
    char ip[16];
    uint64_t timestamps[100];
    int count;
    uint64_t window_start;
} rate_limit_entry_t;

int ratelimit_init(void);
void ratelimit_cleanup(void);
int ratelimit_check(const char *ip, int max_requests, uint64_t window_ms);
void ratelimit_reset(const char *ip);
void ratelimit_cleanup_old(void);
void ratelimit_print_stats(void);

#endif
