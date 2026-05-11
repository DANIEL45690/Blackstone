#include "random.h"

UltraFastRandom::UltraFastRandom()
    : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

std::string UltraFastRandom::bytes(size_t n)
{
    std::string res;
    res.reserve(n);
    for (size_t i = 0; i < n; i++)
        res.push_back(static_cast<char>(rng() & 0xFF));
    return res;
}

uint64_t UltraFastRandom::bounded(uint64_t max)
{
    return u64() % max;
}

UltraFastRandom CryptoRandom::ur;

std::string CryptoRandom::generateKey(size_t bytes)
{
    return ur.bytes(bytes);
}

std::string CryptoRandom::generateIV(size_t bytes)
{
    return ur.bytes(bytes);
}

uint64_t CryptoRandom::generateNonce()
{
    return ur.u64();
}

std::string CryptoRandom::generateSessionId()
{
    return sha1(ur.bytes(32));
}
