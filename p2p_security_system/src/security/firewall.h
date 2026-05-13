#ifndef P2P_SECURITY_FIREWALL_H
#define P2P_SECURITY_FIREWALL_H

#include "../include/common.h"

typedef enum
{
    FW_ACTION_ALLOW,
    FW_ACTION_DENY,
    FW_ACTION_LOG
} firewall_action_t;

typedef struct
{
    char ip[16];
    int port;
    firewall_action_t action;
    int priority;
} firewall_rule_t;

int firewall_init(void);
void firewall_cleanup(void);
int firewall_add_rule(const char *ip, int port, firewall_action_t action, int priority);
int firewall_remove_rule(const char *ip, int port);
firewall_action_t firewall_check(const char *ip, int port);
void firewall_print_rules(void);
void firewall_clear_rules(void);

#endif
