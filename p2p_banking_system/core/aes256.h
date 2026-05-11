#ifndef AES256_H
#define AES256_H

#include <string>
#include <array>
#include <cstdint>
#include <cstring>

class AdvancedAES256
{
    std::array<uint8_t, 32> key;
    bool initialized;
    static const uint8_t sbox[256];
    static const uint8_t invSbox[256];
    static const uint8_t rcon[11];

    void subBytes(uint8_t *state) const;
    void invSubBytes(uint8_t *state) const;
    void shiftRows(uint8_t *state) const;
    void invShiftRows(uint8_t *state) const;
    void mixColumns(uint8_t *state) const;
    void invMixColumns(uint8_t *state) const;
    void addRoundKey(uint8_t *state, const uint8_t *roundKey) const;
    void expandKey(std::array<uint8_t, 176> &expandedKey) const;

public:
    AdvancedAES256();
    AdvancedAES256(const std::string &keyMaterial);
    void setKey(const std::string &keyMaterial);
    std::string encryptECB(const std::string &plain);
    std::string decryptECB(const std::string &cipher);
    std::string encryptCBC(const std::string &plain, const std::string &iv);
    std::string decryptCBC(const std::string &cipher, const std::string &iv);
    std::string encryptGCM(const std::string &plain);
    std::string decryptGCM(const std::string &cipher);
    std::string encryptCTR(const std::string &plain);
    std::string decryptCTR(const std::string &cipher);
    bool isInitialized() const { return initialized; }
};

#endif
