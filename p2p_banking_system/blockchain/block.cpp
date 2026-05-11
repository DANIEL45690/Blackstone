#include "block.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <sstream>

Block::Block() : index(0), timestamp(0), nonce(0), difficulty(4) {}

Block::Block(int idx, const std::string &prevHash)
    : index(idx), previousHash(prevHash), timestamp(getTimestampMs()), nonce(0), difficulty(4) {}

std::string Block::getHeaderData() const
{
    std::stringstream ss;
    ss << index << previousHash << timestamp << merkleRoot << nonce << validator << difficulty << version;
    return ss.str();
}

std::string Block::calculateHash() const
{
    return sha1_double(getHeaderData());
}

bool Block::mineBlock(uint64_t targetDifficulty, uint64_t maxAttempts)
{
    UltraFastRandom rng;
    difficulty = targetDifficulty;
    for (uint64_t attempt = 0; attempt < maxAttempts; attempt++)
    {
        nonce = rng.u64();
        hash = calculateHash();
        std::string prefix = hash.substr(0, targetDifficulty);
        bool found = true;
        for (size_t i = 0; i < targetDifficulty; i++)
        {
            if (prefix[i] != '0')
            {
                found = false;
                break;
            }
        }
        if (found)
            return true;
    }
    return false;
}

bool Block::validateBlock(const Block &previousBlock) const
{
    if (index != previousBlock.index + 1)
        return false;
    if (previousHash != previousBlock.hash)
        return false;
    if (timestamp <= previousBlock.timestamp)
        return false;
    if (calculateHash() != hash)
        return false;
    std::string prefix = hash.substr(0, difficulty);
    for (size_t i = 0; i < difficulty; i++)
    {
        if (prefix[i] != '0')
            return false;
    }
    return true;
}

std::string Block::serialize() const
{
    std::stringstream ss;
    ss << index << "|" << previousHash << "|" << timestamp << "|" << merkleRoot << "|"
       << hash << "|" << nonce << "|" << validator << "|" << difficulty << "|" << version;
    for (const auto &tx : transactions)
    {
        ss << "|" << BankTransaction::serialize(tx);
    }
    return ss.str();
}

Block Block::deserialize(const std::string &data)
{
    Block block;
    std::stringstream ss(data);
    std::string indexStr;
    std::getline(ss, indexStr, '|');
    block.index = std::stoi(indexStr);
    std::getline(ss, block.previousHash, '|');
    std::string tsStr;
    std::getline(ss, tsStr, '|');
    block.timestamp = std::stoull(tsStr);
    std::getline(ss, block.merkleRoot, '|');
    std::getline(ss, block.hash, '|');
    std::string nonceStr;
    std::getline(ss, nonceStr, '|');
    block.nonce = std::stoull(nonceStr);
    std::getline(ss, block.validator, '|');
    std::string diffStr;
    std::getline(ss, diffStr, '|');
    block.difficulty = std::stoull(diffStr);
    std::getline(ss, block.version, '|');
    std::string txData;
    while (std::getline(ss, txData, '|'))
    {
        if (!txData.empty())
        {
            block.transactions.push_back(BankTransaction::deserialize(txData));
        }
    }
    return block;
}

std::string BlockHeader::hash() const
{
    std::stringstream ss;
    ss << version << previousBlockHash << merkleRootHash << timestamp << difficultyTarget << nonce;
    return sha1_double(ss.str());
}

std::string BlockHeader::serialize() const
{
    std::stringstream ss;
    ss << version << "|" << previousBlockHash << "|" << merkleRootHash << "|"
       << timestamp << "|" << difficultyTarget << "|" << nonce;
    return ss.str();
}

BlockHeader BlockHeader::deserialize(const std::string &data)
{
    BlockHeader header;
    std::stringstream ss(data);
    std::string verStr;
    std::getline(ss, verStr, '|');
    header.version = std::stoi(verStr);
    std::getline(ss, header.previousBlockHash, '|');
    std::getline(ss, header.merkleRootHash, '|');
    std::string tsStr;
    std::getline(ss, tsStr, '|');
    header.timestamp = std::stoull(tsStr);
    std::string diffStr;
    std::getline(ss, diffStr, '|');
    header.difficultyTarget = std::stoull(diffStr);
    std::string nonceStr;
    std::getline(ss, nonceStr, '|');
    header.nonce = std::stoull(nonceStr);
    return header;
}
