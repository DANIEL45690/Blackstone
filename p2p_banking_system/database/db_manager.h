#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <fstream>

class DatabaseManager
{
    std::map<std::string, std::string> data;
    std::map<std::string, uint64_t> expiry;
    std::mutex dbMutex;
    std::string dbPath;
    bool autoSave;

    void loadFromDisk();
    void saveToDisk();
    void cleanupExpired();

public:
    explicit DatabaseManager(const std::string &path = "banking.db", bool autoSaveEnabled = true);
    ~DatabaseManager();

    bool set(const std::string &key, const std::string &value, uint64_t ttlMs = 0);
    std::string get(const std::string &key);
    bool del(const std::string &key);
    bool exists(const std::string &key) const;
    std::vector<std::string> keys(const std::string &pattern = "") const;
    size_t size() const;
    void flush();
    void close();
};

#endif
