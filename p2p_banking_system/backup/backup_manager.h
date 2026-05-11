#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

class BackupManager
{
    std::string backupDir;
    std::map<std::string, std::string> metadata;
    std::mutex backupMutex;
    size_t maxBackups;

    std::string generateBackupPath(const std::string &name) const;
    bool compressFile(const std::string &src, const std::string &dst);
    bool decompressFile(const std::string &src, const std::string &dst);

public:
    explicit BackupManager(const std::string &directory, size_t maxBackupCount = 10);

    bool createBackup(const std::string &name, const std::string &data);
    bool restoreBackup(const std::string &name, std::string &data);
    bool deleteBackup(const std::string &name);
    std::vector<std::string> listBackups() const;
    bool backupExists(const std::string &name) const;
    uint64_t getBackupSize(const std::string &name) const;
    uint64_t getBackupTime(const std::string &name) const;
    void cleanupOldBackups();
    void setMaxBackups(size_t max);

    bool exportBackup(const std::string &name, const std::string &exportPath);
    bool importBackup(const std::string &importPath, const std::string &newName);
};

class AutoBackupScheduler
{
    BackupManager &backupManager;
    std::vector<std::string> backupNames;
    std::vector<int> intervalsMs;
    std::vector<std::function<std::string()>> dataProviders;
    std::thread worker;
    std::atomic<bool> running;

    void loop();

public:
    explicit AutoBackupScheduler(BackupManager &manager);
    ~AutoBackupScheduler();

    void addBackupTask(const std::string &name, int intervalMs, std::function<std::string()> dataProvider);
    void removeBackupTask(const std::string &name);
    void start();
    void stop();
};

#endif  
