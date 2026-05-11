#ifndef P2P_HASH_H
#define P2P_HASH_H

#include "../common/types.h"

void hash_compute(const uint8_t* data, size_t len, hash_t* out);
void hash_compute_fast(const uint8_t* data, size_t len, hash_t* out);
bool hash_verify(const hash_t* hash, const uint8_t* data, size_t len);

#endif