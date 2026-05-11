#ifndef ECC_H
#define ECC_H

#include <string>
#include <cstdint>
#include <utility>

class EllipticCurveCryptography
{
    uint64_t privateKey;
    uint64_t publicKey;
    static uint64_t modPow(uint64_t a, uint64_t e, uint64_t m);
    static uint64_t modInverse(uint64_t a, uint64_t m);

public:
    static const uint64_t PRIME = 0xFFFFFFFFFFFFFFC5ull;
    static const uint64_t BASE = 5;
    static const uint64_t ORDER = 0xFFFFFFFFFFFFFFC4ull;

    EllipticCurveCryptography();
    EllipticCurveCryptography(uint64_t privKey);
    uint64_t getPublic() const;
    uint64_t getPrivate() const;
    uint64_t computeSharedSecret(uint64_t otherPublic) const;
    static std::string deriveSessionKey(uint64_t shared);
    std::pair<uint64_t, uint64_t> sign(const std::string &msg) const;
    bool verify(const std::string &msg, uint64_t r1, uint64_t s, uint64_t pubKey) const;
    void generateNewKey();
};

#endif
