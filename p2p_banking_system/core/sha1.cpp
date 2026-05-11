#include "sha1.h"
#include <cstring>

UltraFastSHA1::UltraFastSHA1() : msgLen(0)
{
    reset();
}

void UltraFastSHA1::reset()
{
    h[0] = 0x67452301;
    h[1] = 0xEFCDAB89;
    h[2] = 0x98BADCFE;
    h[3] = 0x10325476;
    h[4] = 0xC3D2E1F0;
    msgLen = 0;
    buffer.clear();
}

void UltraFastSHA1::transform(const uint8_t *chunk)
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

void UltraFastSHA1::update(const std::string &data)
{
    update(reinterpret_cast<const uint8_t *>(data.data()), data.size());
}

void UltraFastSHA1::update(const uint8_t *data, size_t len)
{
    buffer.insert(buffer.end(), data, data + len);
    msgLen += len;
    while (buffer.size() >= 64)
    {
        transform(buffer.data());
        buffer.erase(buffer.begin(), buffer.begin() + 64);
    }
}

std::string UltraFastSHA1::final()
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

std::string sha1(const std::string &s)
{
    UltraFastSHA1 ctx;
    ctx.update(s);
    return ctx.final();
}

std::string sha1_double(const std::string &s)
{
    return sha1(sha1(s));
}

std::string sha1_hmac(const std::string &key, const std::string &msg)
{
    return sha1(key + msg + key);
}

std::string sha1_hmac_raw(const std::string &key, const std::string &msg)
{
    return sha1(key + msg);
}

std::string sha256(const std::string &s)
{
    return sha1_double(s + "256" + s);
}

std::string sha3_256(const std::string &s)
{
    return sha256(sha256(s));
}
