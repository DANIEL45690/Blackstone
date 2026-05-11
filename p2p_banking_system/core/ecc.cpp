#include "ecc.h"
#include "random.h"
#include "sha1.h"

uint64_t EllipticCurveCryptography::modPow(uint64_t a, uint64_t e, uint64_t m)
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

uint64_t EllipticCurveCryptography::modInverse(uint64_t a, uint64_t m)
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

EllipticCurveCryptography::EllipticCurveCryptography()
{
    generateNewKey();
}

EllipticCurveCryptography::EllipticCurveCryptography(uint64_t privKey) : privateKey(privKey)
{
    publicKey = modPow(BASE, privateKey, PRIME);
}

void EllipticCurveCryptography::generateNewKey()
{
    UltraFastRandom r;
    privateKey = r.u64() % ORDER;
    publicKey = modPow(BASE, privateKey, PRIME);
}

uint64_t EllipticCurveCryptography::getPublic() const
{
    return publicKey;
}

uint64_t EllipticCurveCryptography::getPrivate() const
{
    return privateKey;
}

uint64_t EllipticCurveCryptography::computeSharedSecret(uint64_t otherPublic) const
{
    return modPow(otherPublic, privateKey, PRIME);
}

std::string EllipticCurveCryptography::deriveSessionKey(uint64_t shared)
{
    std::stringstream ss;
    ss << std::hex << shared;
    return sha1_double(ss.str() + std::to_string(shared));
}

std::pair<uint64_t, uint64_t> EllipticCurveCryptography::sign(const std::string &msg) const
{
    UltraFastRandom r;
    uint64_t k = r.u64() % ORDER;
    uint64_t r1 = modPow(BASE, k, PRIME);
    std::string hash = sha1(msg + std::to_string(privateKey));
    uint64_t s = (std::stoull(hash.substr(0, 16), nullptr, 16) + privateKey * r1) % ORDER;
    s = (s * modInverse(k, ORDER)) % ORDER;
    return {r1, s};
}

bool EllipticCurveCryptography::verify(const std::string &msg, uint64_t r1, uint64_t s, uint64_t pubKey) const
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
