#ifndef INDEXER_H
#define INDEXER_H

#include <string>
#include <map>
#include <set>
#include <vector>

class Indexer
{
    std::map<std::string, std::map<std::string, std::set<std::string>>> indices;
    std::mutex indexMutex;

public:
    void addToIndex(const std::string &indexName, const std::string &indexValue, const std::string &recordId);
    void removeFromIndex(const std::string &indexName, const std::string &indexValue, const std::string &recordId);
    std::set<std::string> queryIndex(const std::string &indexName, const std::string &indexValue) const;
    std::set<std::string> queryRange(const std::string &indexName, const std::string &start, const std::string &end) const;
    void dropIndex(const std::string &indexName);
    void clear();
    size_t getIndexSize(const std::string &indexName) const;
};

#endif
