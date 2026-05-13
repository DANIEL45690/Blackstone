#ifndef P2P_SECURITY_AUTH_H
#define P2P_SECURITY_AUTH_H

#include "../include/common.h"

typedef struct
{
    char username[64];
    uint8_t password_hash[SHA256_DIGEST_SIZE];
    int role;
    int enabled;
    uint64_t last_login;
} auth_user_t;

int auth_init(void);
void auth_cleanup(void);
int auth_add_user(const char *username, const char *password, int role);
int auth_remove_user(const char *username);
int auth_verify(const char *username, const char *password);
int auth_login(int node_id, const char *username, const char *password);
void auth_logout(int node_id);
void auth_print_users(void);

#endif
