#ifndef P2P_TOKEN_REGISTRY_H
#define P2P_TOKEN_REGISTRY_H

#include "../common/types.h"

typedef struct token_registry {
    token_t* tokens;
    uint32_t count;
    uint32_t capacity;
    CRITICAL_SECTION lock;
    hash_t* hashes;
} token_registry_t;

token_registry_t* token_registry_create(void);
void token_registry_destroy(token_registry_t* registry);
token_id_t token_registry_register(token_registry_t* registry, const token_t* token);
token_t* token_registry_get(token_registry_t* registry, token_id_t id);
token_t* token_registry_find_by_symbol(token_registry_t* registry, const char* symbol);
token_t token_create(const char* symbol, const char* name, const address_t* issuer, amount_t supply, uint8_t decimals);

#endif