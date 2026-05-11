#ifndef CONSENSUS_H
#define CONSENSUS_H

#include "block.h"
#include <vector>
#include <map>
#include <functional>
#include <atomic>

enum class ConsensusType
{
    PROOF_OF_WORK,
    PROOF_OF_STAKE,
    PROOF_OF_AUTHORITY,
    PROOF_OF_ELAPSED_TIME,
    DELEGATED_PROOF_OF_STAKE,
    PRACTICAL_BYZANTINE_FAULT_TOLERANCE
};

class ConsensusEngine
{
    ConsensusType type;
    std::vector<std::string> validators;
    std::map<std::string, double> stakes;
    std::map<std::string, uint64_t> validatorScores;
    std::atomic<uint64_t> currentRound;
    std::mutex consensusMutex;

public:
    ConsensusEngine(ConsensusType ct = ConsensusType::PROOF_OF_WORK);

    void setType(ConsensusType ct);
    void addValidator(const std::string &validatorId, double stake = 0);
    void removeValidator(const std::string &validatorId);
    void updateStake(const std::string &validatorId, double newStake);

    std::string selectValidator(const std::vector<Block> &chain);
    bool validateBlock(const Block &block, const std::vector<Block> &chain);
    uint64_t getCurrentRound() const;

private:
    std::string selectPoWValidator();
    std::string selectPoSValidator();
    std::string selectPoAValidator();
    std::string selectDPoSValidator();
    std::string selectPBFTValidator();
};

class BlockProposer
{
    ConsensusEngine &consensus;
    std::string nodeId;

public:
    BlockProposer(ConsensusEngine &engine, const std::string &id);

    Block proposeBlock(const std::vector<BankTransaction::Transaction> &transactions,
                       const Block &latestBlock, const std::string &validatorId);
    bool canPropose() const;
};

#endif
