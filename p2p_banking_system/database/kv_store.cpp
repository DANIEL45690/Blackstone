#include "kv_store.h"

void KeyValueStore::put(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    store[key] = value;
}

std::string KeyValueStore::get(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = store.find(key);
    if (it != store.end())
        return it->second;
    return "";
}

bool KeyValueStore::remove(const std::string &key)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    return store.erase(key) > 0;
}

bool KeyValueStore::has(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    return store.find(key) != store.end();
}

void KeyValueStore::batchPut(const std::map<std::string, std::string> &items)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    for (const auto &[key, value] : items)
    {
        store[key] = value;
    }
}

std::map<std::string, std::string> KeyValueStore::getRange(const std::string &start, const std::string &end) const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    std::map<std::string, std::string> result;
    for (const auto &[key, value] : store)
    {
        if (key >= start && key <= end)
        {
            result[key] = value;
        }
    }
    return result;
}

void KeyValueStore::addIndex(const std::string &indexName, const std::string &key)
{
    std::lock_guard<std::mutex> lock(storeMutex);
    indexes[indexName].push_back(key);
}

std::vector<std::string> KeyValueStore::getByIndex(const std::string &indexName) const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = indexes.find(indexName);
    if (it != indexes.end())
        return it->second;
    return {};
}

size_t KeyValueStore::size() const
{
    std::lock_guard<std::mutex> lock(storeMutex);
    return store.size();
}

void KeyValueStore::clear()
{
    std::lock_guard<std::mutex> lock(storeMutex);
    store.clear();
    indexes.clear();
}
