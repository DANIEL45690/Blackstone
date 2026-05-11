#ifndef SECURE_LOGGER_H
#define SECURE_LOGGER_H

#include "../core/aes256.h"
#include "../core/ternary.h"
#include <fstream>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>

class SecureLogger
{
    std::ofstream logFile;
    std::mutex logMutex;
    std::queue<std::string> encryptedLogs;
    std::thread encryptionThread;
    std::atomic<bool> running;
    AdvancedAES256 aes;
    std::string currentKey;
    std::string logFilename;

    void encryptAndWrite();

public:
    SecureLogger(const std::string &filename, const std::string &key);
    ~SecureLogger();

    void log(const std::string &level, const std::string &msg);
    void info(const std::string &msg);
    void warn(const std::string &msg);
    void error(const std::string &msg);
    void debug(const std::string &msg);

    void setKey(const std::string &key);
    void rotateLog();
    void flush();
    size_t getQueueSize() const;
};

#endif
