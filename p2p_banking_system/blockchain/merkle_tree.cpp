#include "merkle_tree.h"
#include "../core/sha1.h"
#include <cmath>

MerkleTree::MerkleTree() : built(false) {}

MerkleTree::MerkleTree(const std::vector<std::string> &leafHashes) : leaves(leafHashes), built(false)
{
    rebuild();
}

void MerkleTree::build()
{
    if (leaves.empty())
    {
        root = sha1("empty");
        built = true;
        return;
    }

    tree.clear();
    tree.push_back(leaves);

    std::vector<std::string> currentLevel = leaves;
    while (currentLevel.size() > 1)
    {
        if (currentLevel.size() % 2 == 1)
        {
            currentLevel.push_back(currentLevel.back());
        }
        std::vector<std::string> nextLevel;
        for (size_t i = 0; i < currentLevel.size(); i += 2)
        {
            nextLevel.push_back(hashPair(currentLevel[i], currentLevel[i + 1]));
        }
        tree.push_back(nextLevel);
        currentLevel = nextLevel;
    }

    root = currentLevel.empty() ? sha1("empty") : currentLevel[0];
    built = true;
}

std::string MerkleTree::hashPair(const std::string &left, const std::string &right)
{
    return sha1(left + right);
}

void MerkleTree::addLeaf(const std::string &leafHash)
{
    leaves.push_back(leafHash);
    built = false;
}

void MerkleTree::addLeaves(const std::vector<std::string> &leafHashes)
{
    leaves.insert(leaves.end(), leafHashes.begin(), leafHashes.end());
    built = false;
}

void MerkleTree::rebuild()
{
    build();
}

std::string MerkleTree::getRoot() const
{
    return root;
}

std::vector<std::string> MerkleTree::getProof(size_t leafIndex) const
{
    if (!built || leafIndex >= leaves.size())
        return {};

    std::vector<std::string> proof;
    size_t idx = leafIndex;

    for (size_t level = 0; level < tree.size() - 1; level++)
    {
        size_t siblingIdx = (idx % 2 == 0) ? idx + 1 : idx - 1;
        if (siblingIdx < tree[level].size())
        {
            proof.push_back(tree[level][siblingIdx]);
        }
        idx /= 2;
    }

    return proof;
}

bool MerkleTree::verifyProof(const std::string &leafHash, const std::vector<std::string> &proof, size_t leafIndex) const
{
    return verifyProof(leafHash, proof, root, leafIndex);
}

size_t MerkleTree::getLeafCount() const
{
    return leaves.size();
}

void MerkleTree::clear()
{
    leaves.clear();
    tree.clear();
    root.clear();
    built = false;
}

std::string MerkleTree::computeMerkleRoot(std::vector<std::string> &hashes)
{
    if (hashes.empty())
        return sha1("empty");

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

std::vector<std::string> MerkleTree::computeProof(const std::vector<std::string> &hashes, size_t targetIndex)
{
    if (targetIndex >= hashes.size())
        return {};

    std::vector<std::string> proof;
    std::vector<std::string> currentLevel = hashes;
    size_t idx = targetIndex;

    while (currentLevel.size() > 1)
    {
        size_t siblingIdx = (idx % 2 == 0) ? idx + 1 : idx - 1;
        if (siblingIdx < currentLevel.size())
        {
            proof.push_back(currentLevel[siblingIdx]);
        }

        if (currentLevel.size() % 2 == 1)
        {
            currentLevel.push_back(currentLevel.back());
        }

        std::vector<std::string> nextLevel;
        for (size_t i = 0; i < currentLevel.size(); i += 2)
        {
            nextLevel.push_back(sha1(currentLevel[i] + currentLevel[i + 1]));
        }
        currentLevel = nextLevel;
        idx /= 2;
    }

    return proof;
}

bool MerkleTree::verifyProof(const std::string &hash, const std::vector<std::string> &proof, const std::string &root, size_t index)
{
    std::string current = hash;
    size_t idx = index;

    for (const auto &sibling : proof)
    {
        if (idx % 2 == 0)
        {
            current = sha1(current + sibling);
        }
        else
        {
            current = sha1(sibling + current);
        }
        idx /= 2;
    }

    return current == root;
}
