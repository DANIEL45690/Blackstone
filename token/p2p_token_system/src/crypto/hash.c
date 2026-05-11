#include "hash.h"

void hash_compute(const uint8_t* data, size_t len, hash_t* out) {
    if (!data || !out) return;

    uint64_t h1 = 0xcbf29ce484222325ULL;
    uint64_t h2 = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; i++) {
        h1 ^= data[i];
        h1 *= 0x100000001b3ULL;
        h2 ^= data[i] + 0x9e3779b97f4a7c15ULL;
        h2 *= 0xbf58476d1ce4e5b9ULL;
    }

    memcpy(out->bytes, &h1, 8);
    memcpy(out->bytes + 8, &h2, 8);

    for (int i = 16; i < HASH_LEN; i++) {
        out->bytes[i] = out->bytes[i - 16] ^ out->bytes[i - 8];
    }
}

void hash_compute_fast(const uint8_t* data, size_t len, hash_t* out) {
    if (!data || !out) return;

    uint64_t h = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 0x100000001b3ULL;
    }

    memcpy(out->bytes, &h, 8);

    for (int i = 8; i < HASH_LEN; i++) {
        out->bytes[i] = out->bytes[i - 8] ^ (i * 0xFF);
    }
}

bool hash_verify(const hash_t* hash, const uint8_t* data, size_t len) {
    if (!hash || !data) return false;

    hash_t computed;
    hash_compute(data, len, &computed);

    return memcmp(hash, &computed, HASH_LEN) == 0;
}