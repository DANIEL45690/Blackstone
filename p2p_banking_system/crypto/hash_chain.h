#ifndef HASH_CHAIN_H
#define HASH_CHAIN_H

#include <string>
#include <vector>
#include <deque>

class HashChain
{
    std::deque<std::string> chain;
    std::string seed;
    size_t maxSize;
    std::string currentHash;

public:
    HashChain(const std::string &initialSeed, size_t capacity = 1000);

    void append(const std::string &data);
    std::string getHash(size_t index) const;
    bool verifyChain() const;
    std::string getCurrentHash() const;
    size_t size() const;
    void clear();
    std::string getProof(size_t index) const;
    bool verifyProof(const std::string &data, const std::string &proof, size_t index) const;

    static std::string linkHash(const std::string &prev, const std::string &data);
};

class BlockchainHashChain
{
    std::vector<std::string> blockHashes;
    std::vector<uint64_t> timestamps;
    std::mutex chainMutex;

public:
    void addBlockHash(const std::string &blockHash);
    bool verifyBlockHash(size_t blockIndex, const std::string &claimedHash) const;
    std::string getGenesisHash() const;
    std::string getLatestHash() const;
    size_t getHeight() const;
    std::vector<std::string> getHashesAfter(size_t startIndex) const;
};

#endif
