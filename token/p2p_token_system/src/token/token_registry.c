#include "token_registry.h"
#include "../crypto/hash.h"

token_registry_t* token_registry_create(void) {
    token_registry_t* registry = (token_registry_t*)calloc(1, sizeof(token_registry_t));
    if (!registry) return NULL;

    registry->capacity = MAX_TOKENS;
    registry->tokens = (token_t*)calloc(registry->capacity, sizeof(token_t));
    registry->hashes = (hash_t*)calloc(registry->capacity, sizeof(hash_t));

    if (!registry->tokens || !registry->hashes) {
        free(registry->tokens);
        free(registry->hashes);
        free(registry);
        return NULL;
    }

    registry->count = 0;
    InitializeCriticalSection(&registry->lock);

    return registry;
}

void token_registry_destroy(token_registry_t* registry) {
    if (!registry) return;

    DeleteCriticalSection(&registry->lock);
    free(registry->tokens);
    free(registry->hashes);
    free(registry);
}

token_id_t token_registry_register(token_registry_t* registry, const token_t* token) {
    if (!registry || !token) return (token_id_t)-1;

    EnterCriticalSection(&registry->lock);

    if (registry->count >= registry->capacity) {
        LeaveCriticalSection(&registry->lock);
        return (token_id_t)-1;
    }

    token_id_t id = registry->count;
    registry->tokens[id] = *token;
    registry->tokens[id].id = id;

    hash_compute((uint8_t*)token, sizeof(token_t), &registry->hashes[id]);
    registry->count++;

    LeaveCriticalSection(&registry->lock);
    return id;
}

token_t* token_registry_get(token_registry_t* registry, token_id_t id) {
    if (!registry || id >= registry->count) return NULL;
    return &registry->tokens[id];
}

token_t* token_registry_find_by_symbol(token_registry_t* registry, const char* symbol) {
    if (!registry || !symbol) return NULL;

    EnterCriticalSection(&registry->lock);

    for (uint32_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->tokens[i].symbol, symbol) == 0) {
            LeaveCriticalSection(&registry->lock);
            return &registry->tokens[i];
        }
    }

    LeaveCriticalSection(&registry->lock);
    return NULL;
}

token_t token_create(const char* symbol, const char* name, const address_t* issuer, amount_t supply, uint8_t decimals) {
    token_t token;
    memset(&token, 0, sizeof(token));

    strncpy(token.symbol, symbol, TOKEN_SYMBOL_LEN - 1);
    strncpy(token.name, name, TOKEN_NAME_LEN - 1);
    token.issuer = *issuer;
    token.total_supply = supply;
    token.circulating_supply = supply;
    token.decimals = decimals;
    token.created_at = time(NULL);
    token.holders = 0;
    token.frozen = false;
    token.mintable = true;

    return token;
}