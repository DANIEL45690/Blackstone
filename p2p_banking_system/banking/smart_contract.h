#ifndef SMART_CONTRACT_H
#define SMART_CONTRACT_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <variant>

class SmartContract
{
    using ValueType = std::variant<int64_t, double, std::string, bool>;

    std::string contractId;
    std::string creator;
    std::string code;
    std::map<std::string, ValueType> state;
    std::vector<std::string> events;
    bool isActive;
    uint64_t createdAt;

public:
    SmartContract();
    SmartContract(const std::string &id, const std::string &creatorId, const std::string &contractCode);

    std::string getContractId() const;
    std::string getCreator() const;
    bool isActiveContract() const;
    void activate();
    void deactivate();

    bool execute(const std::string &function, const std::vector<ValueType> &args);
    ValueType getState(const std::string &key) const;
    void setState(const std::string &key, const ValueType &value);
    std::vector<std::string> getEvents() const;

    std::string serialize() const;
    static SmartContract deserialize(const std::string &data);

private:
    void emitEvent(const std::string &event);
};

class SmartContractEngine
{
    std::map<std::string, SmartContract> contracts;
    std::map<std::string, std::vector<std::string>> creatorContracts;
    std::mutex engineMutex;

public:
    bool deployContract(const SmartContract &contract);
    bool executeContract(const std::string &contractId, const std::string &function, const std::vector<SmartContract::ValueType> &args);
    SmartContract getContract(const std::string &contractId) const;
    std::vector<SmartContract> getContractsByCreator(const std::string &creatorId) const;
    bool suspendContract(const std::string &contractId);
    bool activateContract(const std::string &contractId);
    size_t getContractCount() const;
};

#endif
