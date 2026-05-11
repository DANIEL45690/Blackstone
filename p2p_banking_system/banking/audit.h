#ifndef AUDIT_H
#define AUDIT_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>

class AuditLog
{
    struct AuditEntry
    {
        std::string entryId;
        std::string userId;
        std::string action;
        std::string details;
        uint64_t timestamp;
        std::string ipAddress;
        std::string signature;

        std::string serialize() const;
        static AuditEntry deserialize(const std::string &data);
    };

    std::vector<AuditEntry> entries;
    std::map<std::string, std::vector<std::string>> userEntries;
    std::mutex auditMutex;
    size_t maxEntries;
    std::string auditKey;

    std::string signEntry(const AuditEntry &entry) const;
    bool verifyEntry(const AuditEntry &entry) const;

public:
    explicit AuditLog(size_t max = 100000, const std::string &key = "");

    void log(const std::string &userId, const std::string &action, const std::string &details, const std::string &ipAddress = "");
    std::vector<AuditEntry> getEntriesForUser(const std::string &userId, size_t limit = 100) const;
    std::vector<AuditEntry> getEntriesByAction(const std::string &action, size_t limit = 100) const;
    std::vector<AuditEntry> getEntriesSince(uint64_t timestamp) const;
    bool verifyIntegrity() const;
    size_t getEntryCount() const;
    void exportToFile(const std::string &filename) const;
};

class ComplianceChecker
{
    std::vector<std::pair<std::string, std::function<bool(const AuditLog::AuditEntry &)>>> rules;
    std::mutex rulesMutex;

public:
    void addRule(const std::string &ruleName, std::function<bool(const AuditLog::AuditEntry &)> checker);
    void removeRule(const std::string &ruleName);
    std::vector<std::string> checkCompliance(const AuditLog &log) const;
};

#endif
