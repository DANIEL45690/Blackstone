#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <string>
#include <vector>
#include <memory>

class MerkleTree
{
    std::vector<std::string> leaves;
    std::vector<std::vector<std::string>> tree;
    std::string root;
    bool built;

    void build();
    std::string hashPair(const std::string &left, const std::string &right);

public:
    MerkleTree();
    explicit MerkleTree(const std::vector<std::string> &leafHashes);

    void addLeaf(const std::string &leafHash);
    void addLeaves(const std::vector<std::string> &leafHashes);
    void rebuild();
    std::string getRoot() const;
    std::vector<std::string> getProof(size_t leafIndex) const;
    bool verifyProof(const std::string &leafHash, const std::vector<std::string> &proof, size_t leafIndex) const;
    size_t getLeafCount() const;
    void clear();

    static std::string computeMerkleRoot(std::vector<std::string> &hashes);
    static std::vector<std::string> computeProof(const std::vector<std::string> &hashes, size_t targetIndex);
    static bool verifyProof(const std::string &hash, const std::vector<std::string> &proof, const std::string &root, size_t index);
};

#endif
