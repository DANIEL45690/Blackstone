#include "db_manager.h"
#include "../core/ternary.h"

DatabaseManager::DatabaseManager(const std::string &path, bool autoSaveEnabled)
    : dbPath(path), autoSave(autoSaveEnabled)
{
    loadFromDisk();
}

DatabaseManager::~DatabaseManager()
{
    if (autoSave)
        saveToDisk();
}

void DatabaseManager::loadFromDisk()
{
    std::ifstream file(dbPath);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;
        std::string decoded = TernaryCodingSystem::decodeTernary(line);
        size_t colon = decoded.find(':');
        if (colon != std::string::npos)
        {
            std::string key = decoded.substr(0, colon);
            std::string value = decoded.substr(colon + 1);
            data[key] = value;
        }
    }
    file.close();
}

void DatabaseManager::saveToDisk()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::ofstream file(dbPath);
    for (const auto &[key, value] : data)
    {
        std::string line = key + ":" + value;
        file << TernaryCodingSystem::encodeBinary(line) << "\n";
    }
    file.close();
}

void DatabaseManager::cleanupExpired()
{
    std::lock_guard<std::mutex> lock(dbMutex);
    uint64_t now = getTimestampMs();
    for (auto it = expiry.begin(); it != expiry.end();)
    {
        if (now >= it->second)
        {
            data.erase(it->first);
            it = expiry.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool DatabaseManager::set(const std::string &key, const std::string &value, uint64_t ttlMs)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    data[key] = value;
    if (ttlMs > 0)
    {
        expiry[key] = getTimestampMs() + ttlMs;
    }
    else
    {
        expiry.erase(key);
    }
    if (autoSave)
        saveToDisk();
    return true;
}

std::string DatabaseManager::get(const std::string &key)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    cleanupExpired();
    auto it = data.find(key);
    if (it != data.end())
        return it->second;
    return "";
}

bool DatabaseManager::del(const std::string &key)
{
    std::lock_guard<std::mutex> lock(dbMutex);
    data.erase(key);
    expiry.erase(key);
    if (autoSave)
        saveToDisk();
    return true;
}

bool DatabaseManager::exists(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(dbMutex);
    return data.find(key) != data.end();
}

std::vector<std::string> DatabaseManager::keys(const std::string &pattern) const
{
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::string> result;
    for (const auto &[key, val] : data)
    {
        if (pattern.empty() || key.find(pattern) != std::string::npos)
        {
            result.push_back(key);
        }
    }
    return result;
}

size_t DatabaseManager::size() const
{
    std::lock_guard<std::mutex> lock(dbMutex);
    return data.size();
}

void DatabaseManager::flush()
{
    saveToDisk();
}

void DatabaseManager::close()
{
    saveToDisk();
}
