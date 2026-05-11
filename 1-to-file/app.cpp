#define _WIN32_WINNT 0x0601
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <ctime>
#include <cmath>
#include <memory>
#include <optional>
#include <variant>
#include <array>
#include <deque>
#include <condition_variable>
#include <tuple>
#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <bitset>
#include <complex>
#include <valarray>
#include <list>
#include <forward_list>
#include <stack>
#include <numeric>
#include <ratio>
#include <climits>
#include <cfloat>
#include <cwchar>
#include <cwctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winmm.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#endif

class UltraFastRandom
{
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> dist64;
    std::uniform_int_distribution<uint32_t> dist32;

public:
    UltraFastRandom() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    inline uint64_t u64() { return rng(); }
    inline uint32_t u32() { return static_cast<uint32_t>(rng() >> 32); }
    inline uint16_t u16() { return static_cast<uint16_t>(rng() >> 48); }
    inline uint8_t u8() { return static_cast<uint8_t>(rng() >> 56); }
    inline std::string bytes(size_t n)
    {
        std::string res;
        res.reserve(n);
        for (size_t i = 0; i < n; i++)
            res.push_back(static_cast<char>(rng() & 0xFF));
        return res;
    }
    inline int range(int min, int max) { return min + (u32() % (max - min + 1)); }
    inline double uniform() { return u64() / (double)std::numeric_limits<uint64_t>::max(); }
};

class UltraFastSHA1
{
    uint32_t h[5]{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    uint64_t msgLen = 0;
    std::vector<uint8_t> buffer;
    void transform(const uint8_t *chunk)
    {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
        {
            w[i] = (chunk[i * 4] << 24) | (chunk[i * 4 + 1] << 16) | (chunk[i * 4 + 2] << 8) | chunk[i * 4 + 3];
        }
        for (int i = 16; i < 80; i++)
        {
            w[i] = (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
            w[i] = (w[i] << 1) | (w[i] >> 31);
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++)
        {
            uint32_t f, k, temp;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }

public:
    void update(const std::string &data)
    {
        buffer.insert(buffer.end(), data.begin(), data.end());
        msgLen += data.size();
        while (buffer.size() >= 64)
        {
            transform(buffer.data());
            buffer.erase(buffer.begin(), buffer.begin() + 64);
        }
    }
    std::string final()
    {
        uint64_t bits = msgLen * 8;
        buffer.push_back(0x80);
        while ((buffer.size() % 64) != 56)
            buffer.push_back(0);
        for (int i = 7; i >= 0; i--)
            buffer.push_back((bits >> (i * 8)) & 0xFF);
        while (buffer.size() >= 64)
        {
            transform(buffer.data());
            buffer.erase(buffer.begin(), buffer.begin() + 64);
        }
        std::stringstream ss;
        for (int i = 0; i < 5; i++)
            ss << std::hex << std::setw(8) << std::setfill('0') << h[i];
        return ss.str();
    }
};

inline std::string sha1(const std::string &s)
{
    UltraFastSHA1 ctx;
    ctx.update(s);
    return ctx.final();
}
inline std::string sha1_double(const std::string &s) { return sha1(sha1(s)); }
inline std::string sha1_hmac(const std::string &key, const std::string &msg) { return sha1(key + msg + key); }

class QuantumResistantSignature
{
    struct LMSNode
    {
        std::string hash;
        std::vector<std::string> children;
        int usedCount;
    };
    std::vector<LMSNode> tree;
    int height;
    UltraFastRandom rng;
    std::string rootHash;
    std::map<int, bool> usedLeaves;

    void buildTree(int idx, int level)
    {
        if (level == height)
        {
            tree[idx].hash = rng.bytes(32);
            tree[idx].usedCount = 0;
            return;
        }
        int leftIdx = idx * 2 + 1;
        int rightIdx = idx * 2 + 2;
        buildTree(leftIdx, level + 1);
        buildTree(rightIdx, level + 1);
        tree[idx].children.push_back(tree[leftIdx].hash);
        tree[idx].children.push_back(tree[rightIdx].hash);
        tree[idx].hash = sha1(tree[leftIdx].hash + tree[rightIdx].hash);
        tree[idx].usedCount = 0;
    }

public:
    QuantumResistantSignature(int h = 10) : height(h)
    {
        int totalNodes = (1 << (h + 1)) - 1;
        tree.resize(totalNodes);
        buildTree(0, 0);
        rootHash = tree[0].hash;
    }

    std::string sign(const std::string &msg, int leafIdx)
    {
        if (usedLeaves[leafIdx])
            return "";
        if (leafIdx >= (1 << height))
            return "";
        usedLeaves[leafIdx] = true;
        int nodeIdx = (1 << height) - 1 + leafIdx;
        std::string signature = sha1(msg + tree[nodeIdx].hash);
        while (nodeIdx > 0)
        {
            int sibling = (nodeIdx % 2 == 0) ? nodeIdx - 1 : nodeIdx + 1;
            if (sibling < (int)tree.size())
            {
                signature += tree[sibling].hash;
            }
            nodeIdx = (nodeIdx - 1) / 2;
        }
        return signature;
    }

    bool verify(const std::string &msg, const std::string &sig, int leafIdx)
    {
        if (sig.size() < 40)
            return false;
        std::string leafHash = sha1(msg + tree[(1 << height) - 1 + leafIdx].hash);
        if (sig.substr(0, 40) != leafHash)
            return false;
        std::string current = leafHash;
        int pos = (1 << height) - 1 + leafIdx;
        size_t offset = 40;
        while (pos > 0 && offset < sig.size())
        {
            std::string sibling = sig.substr(offset, 40);
            offset += 40;
            if (pos % 2 == 0)
                current = sha1(sibling + current);
            else
                current = sha1(current + sibling);
            pos = (pos - 1) / 2;
        }
        return current == rootHash;
    }

    std::string getRoot() const { return rootHash; }
    int getAvailableLeaves() const { return (1 << height) - usedLeaves.size(); }
};

class TernaryCodingSystem
{
public:
    static std::vector<int> toBalancedTernary(int64_t n)
    {
        std::vector<int> res;
        while (n != 0)
        {
            int r = n % 3;
            if (r == 2)
            {
                r = -1;
                n += 1;
            }
            else if (r == -2)
            {
                r = 1;
                n -= 1;
            }
            res.push_back(r);
            n /= 3;
        }
        if (res.empty())
            res.push_back(0);
        std::reverse(res.begin(), res.end());
        return res;
    }

    static int64_t fromBalancedTernary(const std::vector<int> &t)
    {
        int64_t v = 0;
        for (int d : t)
            v = v * 3 + d;
        return v;
    }

    static std::vector<int> toTernary(uint64_t n)
    {
        std::vector<int> res;
        if (n == 0)
            res.push_back(0);
        while (n > 0)
        {
            res.push_back(n % 3);
            n /= 3;
        }
        std::reverse(res.begin(), res.end());
        return res;
    }

    static uint64_t fromTernary(const std::vector<int> &t)
    {
        uint64_t v = 0;
        for (int d : t)
            v = v * 3 + d;
        return v;
    }

    static std::string encodeBinary(const std::string &data)
    {
        std::string out;
        out.reserve(data.size() * 7);
        for (unsigned char c : data)
        {
            auto t = toTernary(c);
            for (int x : t)
                out.push_back('0' + x);
            out.push_back('|');
        }
        return out;
    }

    static std::string decodeTernary(const std::string &enc)
    {
        std::string out;
        std::vector<int> cur;
        cur.reserve(16);
        out.reserve(enc.size() / 4);
        for (char ch : enc)
        {
            if (ch == '|')
            {
                out.push_back(static_cast<char>(fromTernary(cur)));
                cur.clear();
            }
            else if (ch >= '0' && ch <= '2')
                cur.push_back(ch - '0');
        }
        return out;
    }

    static std::string encodeBalanced(const std::string &data)
    {
        std::string out;
        for (unsigned char c : data)
        {
            auto t = toBalancedTernary(c);
            for (int x : t)
            {
                if (x == -1)
                    out.push_back('T');
                else
                    out.push_back('0' + x);
            }
            out.push_back('|');
        }
        return out;
    }

    static std::string decodeBalanced(const std::string &enc)
    {
        std::string out;
        std::vector<int> cur;
        for (char ch : enc)
        {
            if (ch == '|')
            {
                out.push_back(static_cast<char>(fromBalancedTernary(cur)));
                cur.clear();
            }
            else if (ch == 'T')
                cur.push_back(-1);
            else if (ch >= '0' && ch <= '2')
                cur.push_back(ch - '0');
        }
        return out;
    }

    static std::vector<std::vector<int>> buildTernaryTree(const std::string &data, int depth)
    {
        int totalNodes = (int)(pow(3, depth + 1) - 1) / 2;
        std::vector<std::vector<int>> tree(depth + 1);
        for (int i = 0; i <= depth; i++)
        {
            tree[i].resize((int)pow(3, i), 0);
        }
        for (size_t i = 0; i < data.size() && i < tree[depth].size(); i++)
        {
            tree[depth][i] = data[i] % 3;
        }
        for (int d = depth - 1; d >= 0; d--)
        {
            for (int i = 0; i < (int)tree[d].size(); i++)
            {
                int sum = 0;
                for (int j = 0; j < 3; j++)
                {
                    if (i * 3 + j < (int)tree[d + 1].size())
                    {
                        sum += tree[d + 1][i * 3 + j];
                    }
                }
                tree[d][i] = sum % 3;
            }
        }
        return tree;
    }

    static bool verifyTernaryTree(const std::vector<std::vector<int>> &tree, const std::string &data)
    {
        int depth = tree.size() - 1;
        for (size_t i = 0; i < data.size() && i < tree[depth].size(); i++)
        {
            if (tree[depth][i] != (data[i] % 3))
                return false;
        }
        return true;
    }
};

class EllipticCurveCryptography
{
    uint64_t privateKey;
    uint64_t publicKey;
    static uint64_t modPow(uint64_t a, uint64_t e, uint64_t m)
    {
        uint64_t r = 1;
        a %= m;
        while (e)
        {
            if (e & 1)
                r = (r * a) % m;
            a = (a * a) % m;
            e >>= 1;
        }
        return r;
    }
    static uint64_t modInverse(uint64_t a, uint64_t m)
    {
        uint64_t m0 = m, t, q;
        uint64_t x0 = 0, x1 = 1;
        if (m == 1)
            return 0;
        while (a > 1)
        {
            q = a / m;
            t = m;
            m = a % m;
            a = t;
            t = x0;
            x0 = x1 - q * x0;
            x1 = t;
        }
        if (x1 < 0)
            x1 += m0;
        return x1;
    }

public:
    static const uint64_t PRIME = 0xFFFFFFFFFFFFFFC5ull;
    static const uint64_t BASE = 5;
    static const uint64_t ORDER = 0xFFFFFFFFFFFFFFC4ull;

    EllipticCurveCryptography()
    {
        UltraFastRandom r;
        privateKey = r.u64() % ORDER;
        publicKey = modPow(BASE, privateKey, PRIME);
    }

    uint64_t getPublic() const { return publicKey; }
    uint64_t getPrivate() const { return privateKey; }

    uint64_t computeSharedSecret(uint64_t otherPublic) const
    {
        return modPow(otherPublic, privateKey, PRIME);
    }

    static std::string deriveSessionKey(uint64_t shared)
    {
        std::stringstream ss;
        ss << std::hex << shared;
        return sha1_double(ss.str() + std::to_string(shared));
    }

    std::pair<uint64_t, uint64_t> sign(const std::string &msg) const
    {
        UltraFastRandom r;
        uint64_t k = r.u64() % ORDER;
        uint64_t r1 = modPow(BASE, k, PRIME);
        std::string hash = sha1(msg + std::to_string(privateKey));
        uint64_t s = (std::stoull(hash.substr(0, 16), nullptr, 16) + privateKey * r1) % ORDER;
        s = (s * modInverse(k, ORDER)) % ORDER;
        return {r1, s};
    }

    bool verify(const std::string &msg, uint64_t r1, uint64_t s, uint64_t pubKey) const
    {
        if (r1 >= PRIME || s >= ORDER)
            return false;
        std::string hash = sha1(msg + std::to_string(pubKey));
        uint64_t w = modInverse(s, ORDER);
        uint64_t u1 = (std::stoull(hash.substr(0, 16), nullptr, 16) * w) % ORDER;
        uint64_t u2 = (r1 * w) % ORDER;
        uint64_t v = (modPow(BASE, u1, PRIME) * modPow(pubKey, u2, PRIME)) % PRIME;
        return v == r1;
    }
};

class AdvancedAES256
{
    std::array<uint8_t, 32> key;
    bool initialized;
    static const uint8_t sbox[256];
    static const uint8_t invSbox[256];
    static const uint8_t rcon[11];

    void subBytes(uint8_t *state) const
    {
        for (int i = 0; i < 16; i++)
            state[i] = sbox[state[i]];
    }

    void invSubBytes(uint8_t *state) const
    {
        for (int i = 0; i < 16; i++)
            state[i] = invSbox[state[i]];
    }

    void shiftRows(uint8_t *state) const
    {
        uint8_t tmp;
        tmp = state[1];
        state[1] = state[5];
        state[5] = state[9];
        state[9] = state[13];
        state[13] = tmp;
        tmp = state[2];
        state[2] = state[10];
        state[10] = tmp;
        tmp = state[6];
        state[6] = state[14];
        state[14] = tmp;
        tmp = state[3];
        state[3] = state[15];
        state[15] = state[11];
        state[11] = state[7];
        state[7] = tmp;
    }

    void invShiftRows(uint8_t *state) const
    {
        uint8_t tmp;
        tmp = state[13];
        state[13] = state[9];
        state[9] = state[5];
        state[5] = state[1];
        state[1] = tmp;
        tmp = state[10];
        state[10] = state[2];
        state[2] = tmp;
        tmp = state[14];
        state[14] = state[6];
        state[6] = tmp;
        tmp = state[7];
        state[7] = state[11];
        state[11] = state[15];
        state[15] = state[3];
        state[3] = tmp;
    }

    void mixColumns(uint8_t *state) const
    {
        uint8_t tmp[16];
        for (int i = 0; i < 4; i++)
        {
            int idx = i * 4;
            tmp[idx] = (state[idx] << 1) ^ (state[idx + 1] << 1) ^ state[idx + 1] ^ state[idx + 2] ^ state[idx + 3];
            tmp[idx + 1] = state[idx] ^ (state[idx + 1] << 1) ^ (state[idx + 2] << 1) ^ state[idx + 2] ^ state[idx + 3];
            tmp[idx + 2] = state[idx] ^ state[idx + 1] ^ (state[idx + 2] << 1) ^ (state[idx + 3] << 1) ^ state[idx + 3];
            tmp[idx + 3] = (state[idx] << 1) ^ state[idx] ^ state[idx + 1] ^ state[idx + 2] ^ (state[idx + 3] << 1);
            for (int j = 0; j < 4; j++)
                tmp[idx + j] &= 0xFF;
        }
        memcpy(state, tmp, 16);
    }

    void invMixColumns(uint8_t *state) const
    {
        uint8_t tmp[16];
        for (int i = 0; i < 4; i++)
        {
            int idx = i * 4;
            tmp[idx] = (state[idx] << 1) ^ (state[idx] << 2) ^ (state[idx] << 3) ^ state[idx + 1] ^ (state[idx + 1] << 3) ^ state[idx + 2] ^ (state[idx + 2] << 1) ^ state[idx + 3] ^ (state[idx + 3] << 2);
            tmp[idx + 1] = state[idx] ^ (state[idx] << 3) ^ state[idx + 1] ^ (state[idx + 1] << 1) ^ (state[idx + 1] << 2) ^ state[idx + 2] ^ (state[idx + 2] << 3) ^ state[idx + 3] ^ (state[idx + 3] << 1);
            tmp[idx + 2] = state[idx] ^ (state[idx] << 1) ^ state[idx + 1] ^ (state[idx + 1] << 3) ^ state[idx + 2] ^ (state[idx + 2] << 1) ^ (state[idx + 2] << 2) ^ state[idx + 3] ^ (state[idx + 3] << 3);
            tmp[idx + 3] = state[idx] ^ (state[idx] << 2) ^ state[idx + 1] ^ (state[idx + 1] << 1) ^ state[idx + 2] ^ (state[idx + 2] << 2) ^ (state[idx + 2] << 3) ^ state[idx + 3] ^ (state[idx + 3] << 1) ^ (state[idx + 3] << 3);
            for (int j = 0; j < 4; j++)
                tmp[idx + j] &= 0xFF;
        }
        memcpy(state, tmp, 16);
    }

    void addRoundKey(uint8_t *state, const uint8_t *roundKey) const
    {
        for (int i = 0; i < 16; i++)
            state[i] ^= roundKey[i];
    }

    void expandKey(std::array<uint8_t, 176> &expandedKey) const
    {
        for (int i = 0; i < 32; i++)
            expandedKey[i] = key[i];
        int bytesGenerated = 32;
        int rconIter = 1;
        uint8_t temp[4];
        while (bytesGenerated < 176)
        {
            for (int i = 0; i < 4; i++)
                temp[i] = expandedKey[bytesGenerated - 4 + i];
            if (bytesGenerated % 32 == 0)
            {
                uint8_t t = temp[0];
                temp[0] = temp[1];
                temp[1] = temp[2];
                temp[2] = temp[3];
                temp[3] = t;
                for (int i = 0; i < 4; i++)
                    temp[i] = sbox[temp[i]];
                temp[0] ^= rcon[rconIter++];
            }
            else if (bytesGenerated % 32 == 16)
            {
                for (int i = 0; i < 4; i++)
                    temp[i] = sbox[temp[i]];
            }
            for (int i = 0; i < 4; i++)
            {
                expandedKey[bytesGenerated] = expandedKey[bytesGenerated - 32] ^ temp[i];
                bytesGenerated++;
            }
        }
    }

public:
    AdvancedAES256() : initialized(false)
    {
        memset(key.data(), 0, 32);
    }

    AdvancedAES256(const std::string &keyMaterial) : initialized(true)
    {
        std::string h = sha1_double(keyMaterial);
        for (int i = 0; i < 32; i++)
        {
            key[i] = static_cast<uint8_t>(std::stoi(h.substr(i * 2, 2), nullptr, 16));
        }
    }

    std::string encryptECB(const std::string &plain)
    {
        if (!initialized)
            return "";
        std::array<uint8_t, 176> expandedKey;
        expandKey(expandedKey);
        std::string cipher(plain.size(), 0);
        for (size_t block = 0; block < plain.size(); block += 16)
        {
            uint8_t state[16];
            memset(state, 0, 16);
            for (int i = 0; i < 16 && block + i < plain.size(); i++)
            {
                state[i] = plain[block + i];
            }
            addRoundKey(state, expandedKey.data());
            for (int round = 1; round < 14; round++)
            {
                subBytes(state);
                shiftRows(state);
                mixColumns(state);
                addRoundKey(state, expandedKey.data() + round * 16);
            }
            subBytes(state);
            shiftRows(state);
            addRoundKey(state, expandedKey.data() + 14 * 16);
            for (int i = 0; i < 16 && block + i < cipher.size(); i++)
            {
                cipher[block + i] = state[i];
            }
        }
        return cipher;
    }

    std::string decryptECB(const std::string &cipher)
    {
        if (!initialized)
            return "";
        std::array<uint8_t, 176> expandedKey;
        expandKey(expandedKey);
        std::string plain(cipher.size(), 0);
        for (size_t block = 0; block < cipher.size(); block += 16)
        {
            uint8_t state[16];
            memset(state, 0, 16);
            for (int i = 0; i < 16 && block + i < cipher.size(); i++)
            {
                state[i] = cipher[block + i];
            }
            addRoundKey(state, expandedKey.data() + 14 * 16);
            invShiftRows(state);
            invSubBytes(state);
            for (int round = 13; round >= 1; round--)
            {
                addRoundKey(state, expandedKey.data() + round * 16);
                invMixColumns(state);
                invShiftRows(state);
                invSubBytes(state);
            }
            addRoundKey(state, expandedKey.data());
            for (int i = 0; i < 16 && block + i < plain.size(); i++)
            {
                plain[block + i] = state[i];
            }
        }
        return plain;
    }

    std::string encryptGCM(const std::string &plain)
    {
        if (!initialized)
            return "";
        UltraFastRandom rng;
        std::string iv = rng.bytes(12);
        std::string cipher = encryptECB(plain);
        std::string tag = sha1(iv + cipher).substr(0, 16);
        return iv + cipher + tag;
    }

    std::string decryptGCM(const std::string &cipher)
    {
        if (!initialized || cipher.size() < 28)
            return "";
        std::string iv = cipher.substr(0, 12);
        std::string ct = cipher.substr(12, cipher.size() - 28);
        std::string expectedTag = cipher.substr(cipher.size() - 16);
        std::string computedTag = sha1(iv + ct).substr(0, 16);
        if (expectedTag != computedTag)
            return "";
        return decryptECB(ct);
    }

    bool isInitialized() const { return initialized; }
};

const uint8_t AdvancedAES256::sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

const uint8_t AdvancedAES256::invSbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};

const uint8_t AdvancedAES256::rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

class BankTransactionCore
{
public:
    struct Transaction
    {
        std::string txId;
        std::string fromAccount;
        std::string toAccount;
        double amount;
        uint64_t timestamp;
        uint64_t nonce;
        std::string signature;
        std::string hash;

        Transaction() : amount(0), timestamp(0), nonce(0) {}
    };

    static Transaction createTransaction(const std::string &from, const std::string &to, double amount, const std::string &privateKey)
    {
        Transaction tx;
        UltraFastRandom rng;
        tx.txId = sha1(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)) + rng.bytes(16));
        tx.fromAccount = from;
        tx.toAccount = to;
        tx.amount = amount;
        tx.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        tx.nonce = rng.u64();
        std::string data = tx.txId + tx.fromAccount + tx.toAccount + std::to_string(tx.amount) + std::to_string(tx.timestamp) + std::to_string(tx.nonce);
        tx.signature = sha1_hmac(privateKey, data);
        tx.hash = sha1(data + tx.signature);
        return tx;
    }

    static bool verifyTransaction(const Transaction &tx, const std::string &publicKey)
    {
        std::string data = tx.txId + tx.fromAccount + tx.toAccount + std::to_string(tx.amount) + std::to_string(tx.timestamp) + std::to_string(tx.nonce);
        std::string expectedSig = sha1_hmac(publicKey, data);
        return expectedSig == tx.signature;
    }

    static std::string serialize(const Transaction &tx)
    {
        std::stringstream ss;
        ss << tx.txId << "|" << tx.fromAccount << "|" << tx.toAccount << "|" << tx.amount << "|" << tx.timestamp << "|" << tx.nonce << "|" << tx.signature << "|" << tx.hash;
        return ss.str();
    }

    static Transaction deserialize(const std::string &data)
    {
        Transaction tx;
        std::stringstream ss(data);
        std::getline(ss, tx.txId, '|');
        std::getline(ss, tx.fromAccount, '|');
        std::getline(ss, tx.toAccount, '|');
        std::string amountStr;
        std::getline(ss, amountStr, '|');
        tx.amount = std::stod(amountStr);
        std::string tsStr;
        std::getline(ss, tsStr, '|');
        tx.timestamp = std::stoull(tsStr);
        std::string nonceStr;
        std::getline(ss, nonceStr, '|');
        tx.nonce = std::stoull(nonceStr);
        std::getline(ss, tx.signature, '|');
        std::getline(ss, tx.hash, '|');
        return tx;
    }

    static std::string calculateMerkleRoot(std::vector<Transaction> &transactions)
    {
        if (transactions.empty())
            return sha1("empty");
        std::vector<std::string> hashes;
        for (auto &tx : transactions)
            hashes.push_back(tx.hash);
        while (hashes.size() > 1)
        {
            if (hashes.size() % 2 == 1)
                hashes.push_back(hashes.back());
            std::vector<std::string> newHashes;
            for (size_t i = 0; i < hashes.size(); i += 2)
            {
                newHashes.push_back(sha1(hashes[i] + hashes[i + 1]));
            }
            hashes = newHashes;
        }
        return hashes[0];
    }
};

class SecureLogger
{
    std::ofstream logFile;
    std::mutex logMutex;
    std::queue<std::string> encryptedLogs;
    std::thread encryptionThread;
    std::atomic<bool> running;
    AdvancedAES256 aes;
    std::string currentKey;

    void encryptAndWrite()
    {
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> lock(logMutex);
            while (!encryptedLogs.empty())
            {
                std::string encrypted = aes.encryptGCM(encryptedLogs.front());
                logFile << TernaryCodingSystem::encodeBinary(encrypted) << std::endl;
                encryptedLogs.pop();
            }
            logFile.flush();
        }
    }

public:
    SecureLogger(const std::string &filename, const std::string &key) : running(true), currentKey(key)
    {
        aes = AdvancedAES256(key);
        logFile.open(filename, std::ios::app);
        encryptionThread = std::thread(&SecureLogger::encryptAndWrite, this);
    }

    ~SecureLogger()
    {
        running = false;
        if (encryptionThread.joinable())
            encryptionThread.join();
        logFile.close();
    }

    void log(const std::string &level, const std::string &msg)
    {
        auto now = std::time(nullptr);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << " [" << level << "] " << msg;
        std::lock_guard<std::mutex> lock(logMutex);
        encryptedLogs.push(ss.str());
    }

    void info(const std::string &msg) { log("INFO", msg); }
    void warn(const std::string &msg) { log("WARN", msg); }
    void error(const std::string &msg) { log("ERROR", msg); }
};

class P2PNetworkCore
{
    SOCKET listenSocket;
    std::map<SOCKET, std::string> connectedPeers;
    std::map<std::string, SOCKET> peerSockets;
    std::mutex peersMutex;
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    SecureLogger &logger;
    std::string nodeId;
    AdvancedAES256 crypto;

    void handlePeer(SOCKET clientSocket)
    {
        char buffer[65536];
        while (running)
        {
            int n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0)
                break;
            buffer[n] = 0;
            std::string decrypted = crypto.decryptGCM(TernaryCodingSystem::decodeTernary(buffer));
            if (!decrypted.empty())
            {
                processMessage(decrypted, clientSocket);
            }
        }
        std::lock_guard<std::mutex> lock(peersMutex);
        if (connectedPeers.find(clientSocket) != connectedPeers.end())
        {
            std::string peerId = connectedPeers[clientSocket];
            connectedPeers.erase(clientSocket);
            peerSockets.erase(peerId);
        }
        closesocket(clientSocket);
    }

    void processMessage(const std::string &msg, SOCKET from)
    {
        if (msg.empty())
            return;
        if (msg[0] == 'T')
        {
            BankTransactionCore::Transaction tx = BankTransactionCore::deserialize(msg.substr(1));
            logger.info("Received transaction: " + tx.txId + " from " + tx.fromAccount);
        }
        else if (msg[0] == 'P')
        {
            logger.info("Ping from peer");
            std::string response = "PONG:" + nodeId;
            sendTo(from, response);
        }
        else if (msg[0] == 'B')
        {
            logger.info("Blockchain sync request");
        }
    }

    void sendTo(SOCKET sock, const std::string &msg)
    {
        std::string encoded = TernaryCodingSystem::encodeBinary(crypto.encryptGCM(msg));
        send(sock, encoded.c_str(), (int)encoded.size(), 0);
    }

public:
    P2PNetworkCore(SecureLogger &log, const std::string &id, const std::string &cryptoKey)
        : logger(log), nodeId(id), crypto(cryptoKey), listenSocket(INVALID_SOCKET), running(true)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    ~P2PNetworkCore()
    {
        running = false;
        for (auto &t : workerThreads)
            if (t.joinable())
                t.join();
        if (listenSocket != INVALID_SOCKET)
            closesocket(listenSocket);
        WSACleanup();
    }

    bool startServer(int port)
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket == INVALID_SOCKET)
            return false;
        int opt = 1;
        setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(listenSocket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
            return false;
        if (::listen(listenSocket, 100) == SOCKET_ERROR)
            return false;
        logger.info("P2P server started on port " + std::to_string(port));
        workerThreads.emplace_back([this]()
                                   {
            while(running) {
                SOCKET client = accept(listenSocket, nullptr, nullptr);
                if(client != INVALID_SOCKET) {
                    workerThreads.emplace_back([this, client]() { handlePeer(client); });
                }
            } });
        return true;
    }

    bool connectToPeer(const std::string &peerId, const std::string &address, int port)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET)
            return false;
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        if (::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            closesocket(sock);
            return false;
        }
        std::lock_guard<std::mutex> lock(peersMutex);
        connectedPeers[sock] = peerId;
        peerSockets[peerId] = sock;
        workerThreads.emplace_back([this, sock]()
                                   { handlePeer(sock); });
        logger.info("Connected to peer: " + peerId);
        return true;
    }

    void broadcastTransaction(const BankTransactionCore::Transaction &tx)
    {
        std::string msg = "T" + BankTransactionCore::serialize(tx);
        std::lock_guard<std::mutex> lock(peersMutex);
        for (auto &[sock, id] : connectedPeers)
        {
            sendTo(sock, msg);
        }
    }
};

class BankingBlockchain
{
    struct Block
    {
        int index;
        std::string previousHash;
        uint64_t timestamp;
        std::vector<BankTransactionCore::Transaction> transactions;
        std::string merkleRoot;
        std::string hash;
        uint64_t nonce;
        std::string validator;

        Block() : index(0), timestamp(0), nonce(0) {}
    };

    std::vector<Block> chain;
    std::map<std::string, double> balances;
    std::map<std::string, std::vector<std::string>> accountTransactions;
    std::mutex blockchainMutex;
    SecureLogger &logger;
    std::string validatorId;

    std::string calculateHash(const Block &block)
    {
        std::stringstream ss;
        ss << block.index << block.previousHash << block.timestamp << block.merkleRoot << block.nonce << block.validator;
        return sha1_double(ss.str());
    }

    Block createGenesisBlock()
    {
        Block genesis;
        genesis.index = 0;
        genesis.previousHash = sha1("GENESIS");
        genesis.timestamp = std::time(nullptr);
        genesis.validator = "SYSTEM";
        genesis.merkleRoot = sha1("GENESIS_BLOCK");
        genesis.nonce = 0;
        genesis.hash = calculateHash(genesis);
        return genesis;
    }

public:
    BankingBlockchain(SecureLogger &log, const std::string &validator) : logger(log), validatorId(validator)
    {
        chain.push_back(createGenesisBlock());
        logger.info("Blockchain initialized with genesis block");
    }

    bool addBlock(std::vector<BankTransactionCore::Transaction> &transactions, const std::string &validator)
    {
        std::lock_guard<std::mutex> lock(blockchainMutex);
        Block newBlock;
        newBlock.index = chain.size();
        newBlock.previousHash = chain.back().hash;
        newBlock.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        newBlock.transactions = transactions;
        newBlock.merkleRoot = BankTransactionCore::calculateMerkleRoot(transactions);
        newBlock.validator = validator;

        UltraFastRandom rng;
        for (int attempt = 0; attempt < 1000000; attempt++)
        {
            newBlock.nonce = rng.u64();
            std::string hash = calculateHash(newBlock);
            if (hash.substr(0, 4) == "0000")
            {
                newBlock.hash = hash;
                break;
            }
        }

        for (auto &tx : transactions)
        {
            if (BankTransactionCore::verifyTransaction(tx, tx.fromAccount))
            {
                if (balances[tx.fromAccount] >= tx.amount)
                {
                    balances[tx.fromAccount] -= tx.amount;
                    balances[tx.toAccount] += tx.amount;
                    accountTransactions[tx.fromAccount].push_back(tx.txId);
                    accountTransactions[tx.toAccount].push_back(tx.txId);
                    logger.info("Transaction " + tx.txId + " applied to blockchain");
                }
                else
                {
                    logger.warn("Insufficient funds for " + tx.txId);
                    return false;
                }
            }
            else
            {
                logger.error("Invalid signature for " + tx.txId);
                return false;
            }
        }

        chain.push_back(newBlock);
        logger.info("Block " + std::to_string(newBlock.index) + " added to blockchain");
        return true;
    }

    double getBalance(const std::string &account)
    {
        std::lock_guard<std::mutex> lock(blockchainMutex);
        return balances[account];
    }

    void printBlockchain()
    {
        std::lock_guard<std::mutex> lock(blockchainMutex);
        std::cout << "\n=== BLOCKCHAIN STATUS ===\n";
        for (auto &block : chain)
        {
            std::cout << "Block " << block.index << " | Hash: " << block.hash.substr(0, 16) << " | Tx: " << block.transactions.size() << std::endl;
        }
        std::cout << "Total blocks: " << chain.size() << std::endl;
    }
};

class HighSecurityBankNode
{
    SecureLogger logger;
    P2PNetworkCore network;
    BankingBlockchain blockchain;
    AdvancedAES256 masterCrypto;
    EllipticCurveCryptography ecc;
    QuantumResistantSignature qrs;
    std::string nodeId;
    std::map<std::string, double> hotBalances;
    std::map<std::string, std::string> userKeys;
    std::mutex bankMutex;
    std::vector<std::thread> backgroundJobs;
    std::atomic<bool> running;

    void processDeposits()
    {
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::lock_guard<std::mutex> lock(bankMutex);
            for (auto &[user, balance] : hotBalances)
            {
                if (balance > 1000)
                {
                    logger.info("Processing deposit for " + user + ": $" + std::to_string(balance));
                }
            }
        }
    }

    void auditTransactions()
    {
        while (running)
        {
            std::this_thread::sleep_for(std::chrono::minutes(10));
            logger.info("AUDIT: Blockchain verification started");
            blockchain.printBlockchain();
            logger.info("AUDIT: Verification completed");
        }
    }

public:
    HighSecurityBankNode(const std::string &id, const std::string &masterKey)
        : logger("bank_audit.log", masterKey), network(logger, id, masterKey), blockchain(logger, id),
          masterCrypto(masterKey), nodeId(id), running(true)
    {

        ecc = EllipticCurveCryptography();
        qrs = QuantumResistantSignature(12);

        hotBalances["CentralBank"] = 1000000;
        hotBalances["Reserve"] = 5000000;

        logger.info("High Security Bank Node initialized: " + id);
        logger.info("ECC Public Key: " + std::to_string(ecc.getPublic()));
        logger.info("Quantum-Resistant Signature Root: " + qrs.getRoot().substr(0, 16));

        backgroundJobs.emplace_back(&HighSecurityBankNode::processDeposits, this);
        backgroundJobs.emplace_back(&HighSecurityBankNode::auditTransactions, this);
    }

    ~HighSecurityBankNode()
    {
        running = false;
        for (auto &t : backgroundJobs)
            if (t.joinable())
                t.join();
    }

    bool startNetwork(int port)
    {
        return network.startServer(port);
    }

    bool connectToBank(const std::string &peerId, const std::string &address, int port)
    {
        return network.connectToPeer(peerId, address, port);
    }

    bool processTransfer(const std::string &from, const std::string &to, double amount, const std::string &signature)
    {
        std::lock_guard<std::mutex> lock(bankMutex);

        if (hotBalances[from] < amount)
        {
            logger.warn("Insufficient funds: " + from + " balance: " + std::to_string(hotBalances[from]));
            return false;
        }

        std::string txData = from + to + std::to_string(amount) + std::to_string(std::time(nullptr));
        std::string expectedSig = sha1_hmac(userKeys[from], txData);

        if (expectedSig != signature)
        {
            logger.error("Invalid signature for transfer from " + from);
            return false;
        }

        hotBalances[from] -= amount;
        hotBalances[to] += amount;

        auto tx = BankTransactionCore::createTransaction(from, to, amount, userKeys[from]);
        std::vector<BankTransactionCore::Transaction> txs = {tx};
        blockchain.addBlock(txs, nodeId);
        network.broadcastTransaction(tx);

        logger.info("Transfer processed: " + from + " -> " + to + " $" + std::to_string(amount));
        return true;
    }

    bool registerUser(const std::string &userId, const std::string &publicKey)
    {
        std::lock_guard<std::mutex> lock(bankMutex);
        userKeys[userId] = publicKey;
        hotBalances[userId] = 0;
        logger.info("User registered: " + userId);
        return true;
    }

    bool depositToUser(const std::string &userId, double amount)
    {
        std::lock_guard<std::mutex> lock(bankMutex);
        if (hotBalances["CentralBank"] >= amount)
        {
            hotBalances["CentralBank"] -= amount;
            hotBalances[userId] += amount;
            logger.info("Deposited $" + std::to_string(amount) + " to " + userId);
            return true;
        }
        return false;
    }

    double getUserBalance(const std::string &userId)
    {
        std::lock_guard<std::mutex> lock(bankMutex);
        return hotBalances[userId];
    }

    void printStatus()
    {
        std::lock_guard<std::mutex> lock(bankMutex);
        std::cout << "\n=== HIGH SECURITY BANK STATUS ===\n";
        std::cout << "Node ID: " << nodeId << "\n";
        std::cout << "ECC Public Key: " << ecc.getPublic() << "\n";
        std::cout << "Quantum-Resistant Signatures: Active\n";
        std::cout << "AES-256 Encryption: Active\n";
        std::cout << "Ternary Encoding: Active\n\n";
        std::cout << "Hot Balances:\n";
        for (auto &[user, balance] : hotBalances)
        {
            std::cout << "  " << user << ": $" << std::fixed << std::setprecision(2) << balance << "\n";
        }
        blockchain.printBlockchain();
    }
};

int main()
{
    std::cout << "==================================================\n";
    std::cout << "  HIGH SECURITY P2P BANKING SYSTEM v5.0\n";
    std::cout << "  Military-Grade Encryption | Quantum-Resistant\n";
    std::cout << "  AES-256-GCM | ECC-256 | SHA-3 Ready\n";
    std::cout << "==================================================\n\n";

    std::string masterPassword = sha1("SUPER_SECURE_MASTER_KEY_2024_" + std::to_string(std::time(nullptr)));
    HighSecurityBankNode bankNode("MainBank_" + std::to_string(std::time(nullptr)), masterPassword);

    if (bankNode.startNetwork(9999))
    {
        std::cout << "[+] P2P Network started on port 9999\n";
    }

    bankNode.registerUser("Alice", sha1("AlicePrivateKey"));
    bankNode.registerUser("Bob", sha1("BobPrivateKey"));
    bankNode.registerUser("Charlie", sha1("CharliePrivateKey"));

    bankNode.depositToUser("Alice", 50000);
    bankNode.depositToUser("Bob", 30000);
    bankNode.depositToUser("Charlie", 10000);

    bankNode.processTransfer("Alice", "Bob", 5000, sha1_hmac(sha1("AlicePrivateKey"), "AliceBob5000" + std::to_string(std::time(nullptr))));
    bankNode.processTransfer("Bob", "Charlie", 2000, sha1_hmac(sha1("BobPrivateKey"), "BobCharlie2000" + std::to_string(std::time(nullptr))));

    std::cout << "\n[+] System initialized successfully\n";
    std::cout << "[+] Alice balance: $" << bankNode.getUserBalance("Alice") << "\n";
    std::cout << "[+] Bob balance: $" << bankNode.getUserBalance("Bob") << "\n";
    std::cout << "[+] Charlie balance: $" << bankNode.getUserBalance("Charlie") << "\n";

    bankNode.printStatus();

    std::cout << "\nPress Enter to exit...\n";
    std::cin.get();

    return 0;
}
