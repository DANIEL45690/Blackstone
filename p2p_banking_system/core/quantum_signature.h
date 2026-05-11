#ifndef QUANTUM_SIGNATURE_H
#define QUANTUM_SIGNATURE_H

#include <string>
#include <vector>
#include <map>
#include "random.h"

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

    void buildTree(int idx, int level);

public:
    QuantumResistantSignature(int h = 10);
    std::string sign(const std::string &msg, int leafIdx);
    bool verify(const std::string &msg, const std::string &sig, int leafIdx);
    std::string getRoot() const;
    int getAvailableLeaves() const;
    int getTotalLeaves() const;
    void setLeafUsed(int leafIdx, bool used);
    bool isLeafUsed(int leafIdx) const;
    std::string getLeafHash(int leafIdx) const;
};

#endif
