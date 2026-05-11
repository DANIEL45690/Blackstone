#include "backup_manager.h"
#include "../core/sha1.h"
#include "../core/ternary.h"
#include <fstream>
#include <filesystem>

BackupManager::BackupManager(const std::string &directory, size_t maxBackupCount)
    : backupDir(directory), maxBackups(maxBackupCount)
{
    std::filesystem::create_directories(directory);
}

std::string BackupManager::generateBackupPath(const std::string &name) const
{
    return backupDir + "/" + name + ".backup";
}

bool BackupManager::compressFile(const std::string &src, const std::string &dst)
{
    return true;
}

bool BackupManager::decompressFile(const std::string &src, const std::string &dst)
{
    return true;
}

bool BackupManager::createBackup(const std::string &name, const std::string &data)
{
    std::lock_guard<std::mutex> lock(backupMutex);
    std::string path = generateBackupPath(name);
    std::string encoded = TernaryCodingSystem::encodeBinary(data);

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    file << getTimestampMs() << "\n";
    file << encoded;
    file.close();

    metadata[name] = std::to_string(getTimestampMs());
    cleanupOldBackups();
    return true;
}

bool BackupManager::restoreBackup(const std::string &name, std::string &data)
{
    std::lock_guard<std::mutex> lock(backupMutex);
    std::string path = generateBackupPath(name);

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::string line;
    std::getline(file, line);
    std::string encoded;
    std::getline(file, encoded, '\0');

    data = TernaryCodingSystem::decodeTernary(encoded);
    return !data.empty();
}

bool BackupManager::deleteBackup(const std::string &name)
{
    std::lock_guard<std::mutex> lock(backupMutex);
    std::string path = generateBackupPath(name);
    if (std::filesystem::remove(path))
    {
        metadata.erase(name);
        return true;
    }
    return false;
}

std::vector<std::string> BackupManager::listBackups() const
{
    std::lock_guard<std::mutex> lock(backupMutex);
    std::vector<std::string> result;
    for (const auto &[name, _] : metadata)
    {
        result.push_back(name);
    }
    return result;
}

bool BackupManager::backupExists(const std::string &name) const
{
    std::lock_guard<std::mutex> lock(backupMutex);
    return metadata.find(name) != metadata.end();
}

uint64_t BackupManager::getBackupSize(const std::string &name) const
{
    std::string path = generateBackupPath(name);
    return std::filesystem::file_size(path);
}

uint64_t BackupManager::getBackupTime(const std::string &name) const
{
    std::lock_guard<std::mutex> lock(backupMutex);
    auto it = metadata.find(name);
    if (it != metadata.end())
        return std::stoull(it->second);
    return 0;
}

void BackupManager::cleanupOldBackups()
{
    if (metadata.size() <= maxBackups)
        return;

    std::vector<std::pair<uint64_t, std::string>> sorted;
    for (const auto &[name, timeStr] : metadata)
    {
        sorted.push_back({std::stoull(timeStr), name});
    }
    std::sort(sorted.begin(), sorted.end());

    size_t toDelete = metadata.size() - maxBackups;
    for (size_t i = 0; i < toDelete; i++)
    {
        deleteBackup(sorted[i].second);
    }
}

void BackupManager::setMaxBackups(size_t max)
{
    std::lock_guard<std::mutex> lock(backupMutex);
    maxBackups = max;
    cleanupOldBackups();
}

bool BackupManager::exportBackup(const std::string &name, const std::string &exportPath)
{
    std::string src = generateBackupPath(name);
    std::string dst = exportPath + "/" + name + ".backup";
    try
    {
        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool BackupManager::importBackup(const std::string &importPath, const std::string &newName)
{
    std::string dst = generateBackupPath(newName);
    try
    {
        std::filesystem::copy_file(importPath, dst, std::filesystem::copy_options::overwrite_existing);
        metadata[newName] = std::to_string(getTimestampMs());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

AutoBackupScheduler::AutoBackupScheduler(BackupManager &manager) : backupManager(manager), running(false) {}

AutoBackupScheduler::~AutoBackupScheduler()
{
    stop();
}

void AutoBackupScheduler::addBackupTask(const std::string &name, int intervalMs, std::function<std::string()> dataProvider)
{
    backupNames.push_back(name);
    intervalsMs.push_back(intervalMs);
    dataProviders.push_back(dataProvider);
}

void AutoBackupScheduler::removeBackupTask(const std::string &name)
{
    for (size_t i = 0; i < backupNames.size(); i++)
    {
        if (backupNames[i] == name)
        {
            backupNames.erase(backupNames.begin() + i);
            intervalsMs.erase(intervalsMs.begin() + i);
            dataProviders.erase(dataProviders.begin() + i);
            break;
        }
    }
}

void AutoBackupScheduler::loop()
{
    while (running)
    {
        for (size_t i = 0; i < backupNames.size(); i++)
        {
            std::string data = dataProviders[i]();
            if (!data.empty())
            {
                backupManager.createBackup(backupNames[i], data);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

void AutoBackupScheduler::start()
{
    if (running)
        return;
    running = true;
    worker = std::thread(&AutoBackupScheduler::loop, this);
}

void AutoBackupScheduler::stop()
{
    running = false;
    if (worker.joinable())
        worker.join();
}
