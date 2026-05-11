#include "access_control.h"

Role::Role() {}

Role::Role(const std::string &name) : roleName(name) {}

std::string Role::getName() const { return roleName; }

void Role::addPermission(Permission perm)
{
    permissions.insert(perm);
}

void Role::removePermission(Permission perm)
{
    permissions.erase(perm);
}

bool Role::hasPermission(Permission perm) const
{
    return permissions.find(perm) != permissions.end();
}

std::vector<Permission> Role::getPermissions() const
{
    return std::vector<Permission>(permissions.begin(), permissions.end());
}

AccessController::AccessController()
{
    defineRole("admin", {Permission::READ, Permission::WRITE, Permission::DELETE, Permission::ADMIN,
                         Permission::AUDIT, Permission::MANAGE_USERS, Permission::MANAGE_CONTRACTS});
    defineRole("user", {Permission::READ, Permission::TRANSFER});
    defineRole("auditor", {Permission::READ, Permission::AUDIT, Permission::VIEW_LOGS});
    defineRole("operator", {Permission::READ, Permission::WRITE, Permission::FREEZE_ACCOUNT, Permission::VIEW_LOGS});
}

void AccessController::defineRole(const std::string &roleName, const std::vector<Permission> &permissions)
{
    std::lock_guard<std::mutex> lock(acMutex);
    Role role(roleName);
    for (auto perm : permissions)
    {
        role.addPermission(perm);
    }
    roles[roleName] = role;
}

void AccessController::assignRole(const std::string &userId, const std::string &roleName)
{
    std::lock_guard<std::mutex> lock(acMutex);
    if (roles.find(roleName) != roles.end())
    {
        userRoles[userId] = roleName;
        roleMembers[roleName].insert(userId);
    }
}

void AccessController::revokeRole(const std::string &userId, const std::string &roleName)
{
    std::lock_guard<std::mutex> lock(acMutex);
    auto it = userRoles.find(userId);
    if (it != userRoles.end() && it->second == roleName)
    {
        userRoles.erase(it);
        roleMembers[roleName].erase(userId);
    }
}

bool AccessController::hasPermission(const std::string &userId, Permission perm) const
{
    std::lock_guard<std::mutex> lock(acMutex);
    auto it = userRoles.find(userId);
    if (it == userRoles.end())
        return false;
    auto roleIt = roles.find(it->second);
    if (roleIt == roles.end())
        return false;
    return roleIt->second.hasPermission(perm);
}

std::vector<std::string> AccessController::getUserRoles(const std::string &userId) const
{
    std::lock_guard<std::mutex> lock(acMutex);
    auto it = userRoles.find(userId);
    if (it != userRoles.end())
        return {it->second};
    return {};
}

std::vector<std::string> AccessController::getUsersWithRole(const std::string &roleName) const
{
    std::lock_guard<std::mutex> lock(acMutex);
    auto it = roleMembers.find(roleName);
    if (it != roleMembers.end())
    {
        return std::vector<std::string>(it->second.begin(), it->second.end());
    }
    return {};
}

void AccessController::removeUser(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(acMutex);
    auto it = userRoles.find(userId);
    if (it != userRoles.end())
    {
        roleMembers[it->second].erase(userId);
        userRoles.erase(it);
    }
}
