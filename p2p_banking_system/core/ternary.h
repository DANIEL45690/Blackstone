#ifndef TERNARY_H
#define TERNARY_H

#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

class TernaryCodingSystem
{
public:
    static std::vector<int> toBalancedTernary(int64_t n);
    static int64_t fromBalancedTernary(const std::vector<int> &t);
    static std::vector<int> toTernary(uint64_t n);
    static uint64_t fromTernary(const std::vector<int> &t);
    static std::string encodeBinary(const std::string &data);
    static std::string decodeTernary(const std::string &enc);
    static std::string encodeBalanced(const std::string &data);
    static std::string decodeBalanced(const std::string &enc);
    static std::vector<std::vector<int>> buildTernaryTree(const std::string &data, int depth);
    static bool verifyTernaryTree(const std::vector<std::vector<int>> &tree, const std::string &data);
    static std::string ternaryToBase64(const std::string &ternary);
    static std::string base64ToTernary(const std::string &base64);
    static std::string compressTernary(const std::string &data);
    static std::string decompressTernary(const std::string &compressed);
};

#endif
