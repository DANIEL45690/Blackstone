#include "consensus.h"
#include "../core/random.h"

ConsensusEngine::ConsensusEngine(ConsensusType ct) : type(ct), currentRound(0) {}

void ConsensusEngine::setType(ConsensusType ct)
{
    std::lock_guard<std::mutex> lock(consensusMutex);
    type = ct;
}

void ConsensusEngine::addValidator(const std::string &validatorId, double stake)
{
    std::lock_guard<std::mutex> lock(consensusMutex);
    validators.push_back(validatorId);
    stakes[validatorId] = stake;
    validatorScores[validatorId] = 0;
}

void ConsensusEngine::removeValidator(const std::string &validatorId)
{
    std::lock_guard<std::mutex> lock(consensusMutex);
    for (auto it = validators.begin(); it != validators.end(); ++it)
    {
        if (*it == validatorId)
        {
            validators.erase(it);
            break;
        }
    }
    stakes.erase(validatorId);
    validatorScores.erase(validatorId);
}

void ConsensusEngine::updateStake(const std::string &validatorId, double newStake)
{
    std::lock_guard<std::mutex> lock(consensusMutex);
    stakes[validatorId] = newStake;
}

std::string ConsensusEngine::selectValidator(const std::vector<Block> &chain)
{
    std::lock_guard<std::mutex> lock(consensusMutex);
    currentRound++;

    switch (type)
    {
    case ConsensusType::PROOF_OF_WORK:
        return selectPoWValidator();
    case ConsensusType::PROOF_OF_STAKE:
        return selectPoSValidator();
    case ConsensusType::PROOF_OF_AUTHORITY:
        return selectPoAValidator();
    case ConsensusType::DELEGATED_PROOF_OF_STAKE:
        return selectDPoSValidator();
    case ConsensusType::PRACTICAL_BYZANTINE_FAULT_TOLERANCE:
        return selectPBFTValidator();
    default:
        return selectPoWValidator();
    }
}

bool ConsensusEngine::validateBlock(const Block &block, const std::vector<Block> &chain)
{
    if (block.validator.empty())
        return false;

    bool isValidValidator = false;
    for (const auto &v : validators)
    {
        if (v == block.validator)
        {
            isValidValidator = true;
            break;
        }
    }

    if (!isValidValidator)
        return false;

    if (block.index != (int)chain.size())
        return false;

    return true;
}

uint64_t ConsensusEngine::getCurrentRound() const
{
    return currentRound;
}

std::string ConsensusEngine::selectPoWValidator()
{
    UltraFastRandom rng;
    if (validators.empty())
        return "";
    return validators[rng.u64() % validators.size()];
}

std::string ConsensusEngine::selectPoSValidator()
{
    double totalStake = 0;
    for (const auto &[id, stake] : stakes)
    {
        totalStake += stake;
    }

    if (totalStake == 0)
        return selectPoWValidator();

    UltraFastRandom rng;
    double target = rng.uniform() * totalStake;
    double cumulative = 0;

    for (const auto &[id, stake] : stakes)
    {
        cumulative += stake;
        if (cumulative >= target)
            return id;
    }

    return validators.empty() ? "" : validators[0];
}

std::string ConsensusEngine::selectPoAValidator()
{
    if (validators.empty())
        return "";
    static size_t roundRobinIndex = 0;
    std::string selected = validators[roundRobinIndex % validators.size()];
    roundRobinIndex++;
    return selected;
}

std::string ConsensusEngine::selectDPoSValidator()
{
    return selectPoSValidator();
}

std::string ConsensusEngine::selectPBFTValidator()
{
    if (validators.size() < 4)
        return selectPoWValidator();
    return validators[currentRound % validators.size()];
}

BlockProposer::BlockProposer(ConsensusEngine &engine, const std::string &id)
    : consensus(engine), nodeId(id) {}

Block BlockProposer::proposeBlock(const std::vector<BankTransaction::Transaction> &transactions,
                                  const Block &latestBlock, const std::string &validatorId)
{
    Block newBlock;
    newBlock.index = latestBlock.index + 1;
    newBlock.previousHash = latestBlock.hash;
    newBlock.timestamp = getTimestampMs();
    newBlock.transactions = transactions;
    newBlock.merkleRoot = BankTransaction::calculateMerkleRoot(const_cast<std::vector<BankTransaction::Transaction> &>(transactions));
    newBlock.validator = validatorId;
    newBlock.difficulty = 4;
    newBlock.version = "1.0";

    return newBlock;
}

bool BlockProposer::canPropose() const
{
    return true;
}
