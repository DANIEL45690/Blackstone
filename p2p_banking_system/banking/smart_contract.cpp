#include "smart_contract.h"
#include "../core/sha1.h"

SmartContract::SmartContract() : isActive(false), createdAt(0) {}

SmartContract::SmartContract(const std::string &id, const std::string &creatorId, const std::string &contractCode)
    : contractId(id), creator(creatorId), code(contractCode), isActive(true), createdAt(getTimestampMs()) {}

std::string SmartContract::getContractId() const { return contractId; }
std::string SmartContract::getCreator() const { return creator; }
bool SmartContract::isActiveContract() const { return isActive; }
void SmartContract::activate() { isActive = true; }
void SmartContract::deactivate() { isActive = false; }

bool SmartContract::execute(const std::string &function, const std::vector<ValueType> &args)
{
    if (!isActive)
        return false;
    emitEvent("execute:" + function);
    return true;
}

SmartContract::ValueType SmartContract::getState(const std::string &key) const
{
    auto it = state.find(key);
    if (it != state.end())
        return it->second;
    return std::string("");
}

void SmartContract::setState(const std::string &key, const ValueType &value)
{
    state[key] = value;
    emitEvent("state_change:" + key);
}

std::vector<std::string> SmartContract::getEvents() const
{
    return events;
}

void SmartContract::emitEvent(const std::string &event)
{
    events.push_back(event);
    if (events.size() > 1000)
        events.erase(events.begin());
}

std::string SmartContract::serialize() const
{
    std::stringstream ss;
    ss << contractId << "|" << creator << "|" << code << "|" << isActive << "|" << createdAt;
    for (const auto &[key, val] : state)
    {
        ss << "|" << key << ":";
        if (std::holds_alternative<int64_t>(val))
            ss << "i:" << std::get<int64_t>(val);
        else if (std::holds_alternative<double>(val))
            ss << "d:" << std::get<double>(val);
        else if (std::holds_alternative<std::string>(val))
            ss << "s:" << std::get<std::string>(val);
        else if (std::holds_alternative<bool>(val))
            ss << "b:" << std::get<bool>(val);
    }
    return ss.str();
}

SmartContract SmartContract::deserialize(const std::string &data)
{
    SmartContract contract;
    std::stringstream ss(data);
    std::getline(ss, contract.contractId, '|');
    std::getline(ss, contract.creator, '|');
    std::getline(ss, contract.code, '|');
    std::string activeStr;
    std::getline(ss, activeStr, '|');
    contract.isActive = activeStr == "1";
    std::string createdStr;
    std::getline(ss, createdStr, '|');
    contract.createdAt = std::stoull(createdStr);
    std::string kv;
    while (std::getline(ss, kv, '|'))
    {
        if (kv.empty())
            continue;
        size_t colon = kv.find(':');
        if (colon != std::string::npos)
        {
            std::string key = kv.substr(0, colon);
            std::string value = kv.substr(colon + 1);
            if (value.substr(0, 2) == "i:")
                contract.state[key] = std::stoll(value.substr(2));
            else if (value.substr(0, 2) == "d:")
                contract.state[key] = std::stod(value.substr(2));
            else if (value.substr(0, 2) == "s:")
                contract.state[key] = value.substr(2);
            else if (value.substr(0, 2) == "b:")
                contract.state[key] = value.substr(2) == "1";
        }
    }
    return contract;
}

bool SmartContractEngine::deployContract(const SmartContract &contract)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    if (contracts.find(contract.getContractId()) != contracts.end())
        return false;
    contracts[contract.getContractId()] = contract;
    creatorContracts[contract.getCreator()].push_back(contract.getContractId());
    return true;
}

bool SmartContractEngine::executeContract(const std::string &contractId, const std::string &function, const std::vector<SmartContract::ValueType> &args)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = contracts.find(contractId);
    if (it == contracts.end())
        return false;
    return it->second.execute(function, args);
}

SmartContract SmartContractEngine::getContract(const std::string &contractId) const
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = contracts.find(contractId);
    if (it != contracts.end())
        return it->second;
    return SmartContract();
}

std::vector<SmartContract> SmartContractEngine::getContractsByCreator(const std::string &creatorId) const
{
    std::lock_guard<std::mutex> lock(engineMutex);
    std::vector<SmartContract> result;
    auto it = creatorContracts.find(creatorId);
    if (it != creatorContracts.end())
    {
        for (const auto &contractId : it->second)
        {
            result.push_back(contracts.at(contractId));
        }
    }
    return result;
}

bool SmartContractEngine::suspendContract(const std::string &contractId)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = contracts.find(contractId);
    if (it == contracts.end())
        return false;
    it->second.deactivate();
    return true;
}

bool SmartContractEngine::activateContract(const std::string &contractId)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = contracts.find(contractId);
    if (it == contracts.end())
        return false;
    it->second.activate();
    return true;
}

size_t SmartContractEngine::getContractCount() const
{
    std::lock_guard<std::mutex> lock(engineMutex);
    return contracts.size();
}
