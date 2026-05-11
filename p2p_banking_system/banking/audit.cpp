#include "audit.h"
#include "../core/sha1.h"
#include "../core/random.h"
#include <fstream>

std::string AuditLog::AuditEntry::serialize() const
{
    std::stringstream ss;
    ss << entryId << "|" << userId << "|" << action << "|" << details << "|"
       << timestamp << "|" << ipAddress << "|" << signature;
    return ss.str();
}

AuditLog::AuditEntry AuditLog::AuditEntry::deserialize(const std::string &data)
{
    AuditEntry entry;
    std::stringstream ss(data);
    std::getline(ss, entry.entryId, '|');
    std::getline(ss, entry.userId, '|');
    std::getline(ss, entry.action, '|');
    std::getline(ss, entry.details, '|');
    std::string tsStr;
    std::getline(ss, tsStr, '|');
    entry.timestamp = std::stoull(tsStr);
    std::getline(ss, entry.ipAddress, '|');
    std::getline(ss, entry.signature, '|');
    return entry;
}

std::string AuditLog::signEntry(const AuditEntry &entry) const
{
    std::string data = entry.entryId + entry.userId + entry.action + entry.details +
                       std::to_string(entry.timestamp) + entry.ipAddress;
    return sha1_hmac(auditKey, data);
}

bool AuditLog::verifyEntry(const AuditEntry &entry) const
{
    std::string expected = signEntry(entry);
    return expected == entry.signature;
}

AuditLog::AuditLog(size_t max, const std::string &key) : maxEntries(max), auditKey(key) {}

void AuditLog::log(const std::string &userId, const std::string &action, const std::string &details, const std::string &ipAddress)
{
    std::lock_guard<std::mutex> lock(auditMutex);
    UltraFastRandom rng;
    AuditEntry entry;
    entry.entryId = sha1(userId + action + std::to_string(getTimestampMs()) + rng.bytes(16));
    entry.userId = userId;
    entry.action = action;
    entry.details = details;
    entry.timestamp = getTimestampMs();
    entry.ipAddress = ipAddress;
    entry.signature = signEntry(entry);

    if (entries.size() >= maxEntries)
    {
        entries.erase(entries.begin());
    }
    entries.push_back(entry);
    userEntries[userId].push_back(entry.entryId);
}

std::vector<AuditLog::AuditEntry> AuditLog::getEntriesForUser(const std::string &userId, size_t limit) const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    std::vector<AuditEntry> result;
    auto it = userEntries.find(userId);
    if (it != userEntries.end())
    {
        for (const auto &entryId : it->second)
        {
            for (const auto &entry : entries)
            {
                if (entry.entryId == entryId)
                {
                    result.push_back(entry);
                    break;
                }
            }
        }
    }
    if (result.size() > limit)
        result.resize(limit);
    return result;
}

std::vector<AuditLog::AuditEntry> AuditLog::getEntriesByAction(const std::string &action, size_t limit) const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    std::vector<AuditEntry> result;
    for (const auto &entry : entries)
    {
        if (entry.action == action)
        {
            result.push_back(entry);
        }
    }
    if (result.size() > limit)
        result.resize(limit);
    return result;
}

std::vector<AuditLog::AuditEntry> AuditLog::getEntriesSince(uint64_t timestamp) const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    std::vector<AuditEntry> result;
    for (const auto &entry : entries)
    {
        if (entry.timestamp >= timestamp)
        {
            result.push_back(entry);
        }
    }
    return result;
}

bool AuditLog::verifyIntegrity() const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    for (const auto &entry : entries)
    {
        if (!verifyEntry(entry))
            return false;
    }
    return true;
}

size_t AuditLog::getEntryCount() const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    return entries.size();
}

void AuditLog::exportToFile(const std::string &filename) const
{
    std::lock_guard<std::mutex> lock(auditMutex);
    std::ofstream file(filename);
    for (const auto &entry : entries)
    {
        file << entry.serialize() << "\n";
    }
}

void ComplianceChecker::addRule(const std::string &ruleName, std::function<bool(const AuditLog::AuditEntry &)> checker)
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    rules.emplace_back(ruleName, checker);
}

void ComplianceChecker::removeRule(const std::string &ruleName)
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    for (auto it = rules.begin(); it != rules.end(); ++it)
    {
        if (it->first == ruleName)
        {
            rules.erase(it);
            break;
        }
    }
}

std::vector<std::string> ComplianceChecker::checkCompliance(const AuditLog &log) const
{
    std::lock_guard<std::mutex> lock(rulesMutex);
    std::vector<std::string> violations;
    return violations;
}
