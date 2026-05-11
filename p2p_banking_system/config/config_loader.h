#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <map>
#include <vector>
#include <any>

class ConfigLoader
{
    std::map<std::string, std::string> config;
    std::string configPath;
    std::mutex configMutex;

    void parseLine(const std::string &line);
    void parseJSON(const std::string &content);
    void parseINI(const std::string &content);

public:
    explicit ConfigLoader(const std::string &path = "config.ini");

    bool load();
    bool save();
    bool reload();

    std::string getString(const std::string &key, const std::string &defaultValue = "") const;
    int getInt(const std::string &key, int defaultValue = 0) const;
    double getDouble(const std::string &key, double defaultValue = 0.0) const;
    bool getBool(const std::string &key, bool defaultValue = false) const;

    void setString(const std::string &key, const std::string &value);
    void setInt(const std::string &key, int value);
    void setDouble(const std::string &key, double value);
    void setBool(const std::string &key, bool value);

    bool hasKey(const std::string &key) const;
    void removeKey(const std::string &key);
    std::vector<std::string> getKeys() const;
    void clear();
};

class EnvironmentConfig
{
public:
    static std::string getEnv(const std::string &name, const std::string &defaultValue = "");
    static int getEnvInt(const std::string &name, int defaultValue = 0);
    static bool getEnvBool(const std::string &name, bool defaultValue = false);
    static void setEnv(const std::string &name, const std::string &value);
};

#endif
