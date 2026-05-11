#ifndef SHA1_H
#define SHA1_H

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>

class UltraFastSHA1
{
    uint32_t h[5];
    uint64_t msgLen;
    std::vector<uint8_t> buffer;
    void transform(const uint8_t *chunk);

public:
    UltraFastSHA1();
    void update(const std::string &data);
    void update(const uint8_t *data, size_t len);
    std::string final();
    void reset();
};

std::string sha1(const std::string &s);
std::string sha1_double(const std::string &s);
std::string sha1_hmac(const std::string &key, const std::string &msg);
std::string sha1_hmac_raw(const std::string &key, const std::string &msg);
std::string sha256(const std::string &s);
std::string sha3_256(const std::string &s);

#endif
