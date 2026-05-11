#include "indexer.h"

void Indexer::addToIndex(const std::string &indexName, const std::string &indexValue, const std::string &recordId)
{
    std::lock_guard<std::mutex> lock(indexMutex);
    indices[indexName][indexValue].insert(recordId);
}

void Indexer::removeFromIndex(const std::string &indexName, const std::string &indexValue, const std::string &recordId)
{
    std::lock_guard<std::mutex> lock(indexMutex);
    auto it = indices.find(indexName);
    if (it != indices.end())
    {
        auto &valueMap = it->second;
        auto vit = valueMap.find(indexValue);
        if (vit != valueMap.end())
        {
            vit->second.erase(recordId);
            if (vit->second.empty())
            {
                valueMap.erase(vit);
            }
        }
        if (valueMap.empty())
        {
            indices.erase(it);
        }
    }
}

std::set<std::string> Indexer::queryIndex(const std::string &indexName, const std::string &indexValue) const
{
    std::lock_guard<std::mutex> lock(indexMutex);
    auto it = indices.find(indexName);
    if (it != indices.end())
    {
        auto vit = it->second.find(indexValue);
        if (vit != it->second.end())
        {
            return vit->second;
        }
    }
    return {};
}

std::set<std::string> Indexer::queryRange(const std::string &indexName, const std::string &start, const std::string &end) const
{
    std::lock_guard<std::mutex> lock(indexMutex);
    std::set<std::string> result;
    auto it = indices.find(indexName);
    if (it != indices.end())
    {
        for (const auto &[value, records] : it->second)
        {
            if (value >= start && value <= end)
            {
                result.insert(records.begin(), records.end());
            }
        }
    }
    return result;
}

void Indexer::dropIndex(const std::string &indexName)
{
    std::lock_guard<std::mutex> lock(indexMutex);
    indices.erase(indexName);
}

void Indexer::clear()
{
    std::lock_guard<std::mutex> lock(indexMutex);
    indices.clear();
}

size_t Indexer::getIndexSize(const std::string &indexName) const
{
    std::lock_guard<std::mutex> lock(indexMutex);
    auto it = indices.find(indexName);
    if (it != indices.end())
    {
        size_t total = 0;
        for (const auto &[value, records] : it->second)
        {
            total += records.size();
        }
        return total;
    }
    return 0;
}
