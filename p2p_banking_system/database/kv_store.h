#ifndef KV_STORE_H
#define KV_STORE_H

#include <string>
#include <map>
#include <vector>

class KeyValueStore
{
    std::map<std::string, std::string> store;
    std::map<std::string, std::vector<std::string>> indexes;
    std::mutex storeMutex;

public:
    void put(const std::string &key, const std::string &value);
    std::string get(const std::string &key) const;
    bool remove(const std::string &key);
    bool has(const std::string &key) const;
    void batchPut(const std::map<std::string, std::string> &items);
    std::map<std::string, std::string> getRange(const std::string &start, const std::string &end) const;
    void addIndex(const std::string &indexName, const std::string &key);
    std::vector<std::string> getByIndex(const std::string &indexName) const;
    size_t size() const;
    void clear();
};

#endif
