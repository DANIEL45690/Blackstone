#include "quantum_signature.h"
#include "sha1.h"

QuantumResistantSignature::QuantumResistantSignature(int h) : height(h)
{
    int totalNodes = (1 << (h + 1)) - 1;
    tree.resize(totalNodes);
    buildTree(0, 0);
    rootHash = tree[0].hash;
}

void QuantumResistantSignature::buildTree(int idx, int level)
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

std::string QuantumResistantSignature::sign(const std::string &msg, int leafIdx)
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

bool QuantumResistantSignature::verify(const std::string &msg, const std::string &sig, int leafIdx)
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

std::string QuantumResistantSignature::getRoot() const
{
    return rootHash;
}

int QuantumResistantSignature::getAvailableLeaves() const
{
    return (1 << height) - (int)usedLeaves.size();
}

int QuantumResistantSignature::getTotalLeaves() const
{
    return 1 << height;
}

void QuantumResistantSignature::setLeafUsed(int leafIdx, bool used)
{
    usedLeaves[leafIdx] = used;
}

bool QuantumResistantSignature::isLeafUsed(int leafIdx) const
{
    auto it = usedLeaves.find(leafIdx);
    return it != usedLeaves.end() && it->second;
}

std::string QuantumResistantSignature::getLeafHash(int leafIdx) const
{
    if (leafIdx >= (1 << height))
        return "";
    return tree[(1 << height) - 1 + leafIdx].hash;
}
