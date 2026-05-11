#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#include <string>
#include <map>
#include <set>
#include <vector>

enum class Permission
{
    READ,
    WRITE,
    DELETE,
    ADMIN,
    AUDIT,
    TRANSFER,
    CREATE_ACCOUNT,
    FREEZE_ACCOUNT,
    VIEW_LOGS,
    MANAGE_USERS,
    MANAGE_CONTRACTS
};

class Role
{
    std::string roleName;
    std::set<Permission> permissions;

public:
    Role();
    explicit Role(const std::string &name);

    std::string getName() const;
    void addPermission(Permission perm);
    void removePermission(Permission perm);
    bool hasPermission(Permission perm) const;
    std::vector<Permission> getPermissions() const;
};

class AccessController
{
    std::map<std::string, std::string> userRoles;
    std::map<std::string, Role> roles;
    std::map<std::string, std::set<std::string>> roleMembers;
    std::mutex acMutex;

public:
    AccessController();

    void defineRole(const std::string &roleName, const std::vector<Permission> &permissions);
    void assignRole(const std::string &userId, const std::string &roleName);
    void revokeRole(const std::string &userId, const std::string &roleName);
    bool hasPermission(const std::string &userId, Permission perm) const;
    std::vector<std::string> getUserRoles(const std::string &userId) const;
    std::vector<std::string> getUsersWithRole(const std::string &roleName) const;
    void removeUser(const std::string &userId);

    static bool isAuthorized(const std::string &token, Permission required);
};

#endif
