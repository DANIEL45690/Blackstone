#ifndef RANDOM_H
#define RANDOM_H

#include <random>
#include <chrono>
#include <string>
#include <cstdint>

class UltraFastRandom
{
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> dist64;
    std::uniform_int_distribution<uint32_t> dist32;

public:
    UltraFastRandom();
    inline uint64_t u64() { return rng(); }
    inline uint32_t u32() { return static_cast<uint32_t>(rng() >> 32); }
    inline uint16_t u16() { return static_cast<uint16_t>(rng() >> 48); }
    inline uint8_t u8() { return static_cast<uint8_t>(rng() >> 56); }
    std::string bytes(size_t n);
    inline int range(int min, int max) { return min + (u32() % (max - min + 1)); }
    inline double uniform() { return u64() / (double)std::numeric_limits<uint64_t>::max(); }
    inline double normal(double mean, double stddev)
    {
        std::normal_distribution<double> nd(mean, stddev);
        return nd(rng);
    }
    uint64_t bounded(uint64_t max);
};

class CryptoRandom
{
    static UltraFastRandom ur;

public:
    static std::string generateKey(size_t bytes);
    static std::string generateIV(size_t bytes);
    static uint64_t generateNonce();
    static std::string generateSessionId();
};

#endif
