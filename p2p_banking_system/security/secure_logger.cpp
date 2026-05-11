#include "secure_logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>

SecureLogger::SecureLogger(const std::string &filename, const std::string &key)
    : running(true), currentKey(key), logFilename(filename)
{
    aes = AdvancedAES256(key);
    logFile.open(filename, std::ios::app);
    encryptionThread = std::thread(&SecureLogger::encryptAndWrite, this);
}

SecureLogger::~SecureLogger()
{
    running = false;
    if (encryptionThread.joinable())
        encryptionThread.join();
    logFile.close();
}

void SecureLogger::encryptAndWrite()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lock(logMutex);
        while (!encryptedLogs.empty())
        {
            std::string encrypted = aes.encryptGCM(encryptedLogs.front());
            logFile << TernaryCodingSystem::encodeBinary(encrypted) << std::endl;
            encryptedLogs.pop();
        }
        logFile.flush();
    }
}

void SecureLogger::log(const std::string &level, const std::string &msg)
{
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << " [" << level << "] " << msg;
    std::lock_guard<std::mutex> lock(logMutex);
    encryptedLogs.push(ss.str());

    if (encryptedLogs.size() > 10000)
    {
        std::queue<std::string> empty;
        std::swap(encryptedLogs, empty);
    }
}

void SecureLogger::info(const std::string &msg) { log("INFO", msg); }
void SecureLogger::warn(const std::string &msg) { log("WARN", msg); }
void SecureLogger::error(const std::string &msg) { log("ERROR", msg); }
void SecureLogger::debug(const std::string &msg) { log("DEBUG", msg); }

void SecureLogger::setKey(const std::string &key)
{
    std::lock_guard<std::mutex> lock(logMutex);
    currentKey = key;
    aes.setKey(key);
}

void SecureLogger::rotateLog()
{
    std::lock_guard<std::mutex> lock(logMutex);
    logFile.close();
    std::string newFilename = logFilename + "." + std::to_string(std::time(nullptr));
    std::rename(logFilename.c_str(), newFilename.c_str());
    logFile.open(logFilename, std::ios::app);
}

void SecureLogger::flush()
{
    std::lock_guard<std::mutex> lock(logMutex);
    logFile.flush();
}

size_t SecureLogger::getQueueSize() const
{
    std::lock_guard<std::mutex> lock(logMutex);
    return encryptedLogs.size();
}
