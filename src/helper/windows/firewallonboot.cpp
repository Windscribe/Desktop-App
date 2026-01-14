#include "firewallonboot.h"

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

static const std::wstring kRegistryKey = L"SOFTWARE\\Windscribe\\Windscribe2";
static const std::wstring kRegistryValue = L"FirewallOnBoot";

FirewallOnBootManager::FirewallOnBootManager()
{
}

FirewallOnBootManager::~FirewallOnBootManager()
{
}

bool FirewallOnBootManager::setEnabled(bool enabled)
{
    if (enabled) {
        return enable();
    }
    return disable();
}

bool FirewallOnBootManager::isEnabled()
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, kRegistryKey);
    if (!result) {
        if (result.Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"FirewallOnBootManager::isEnabled failed opening key {}: error {}", kRegistryKey, result.Code());
        }
        return false;
    }

    const auto value = registry.TryGetDwordValue(kRegistryValue);
    if (!value) {
        if (value.GetError().Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"FirewallOnBootManager::isEnabled failed checking value {}: error {}", kRegistryValue, value.GetError().Code());
        }
        return false;
    }

    return (value.GetValue() == 1);
}

bool FirewallOnBootManager::enable()
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryCreate(HKEY_LOCAL_MACHINE, kRegistryKey);
    if (!result) {
        spdlog::error(L"FirewallOnBootManager::enable failed creating key {}: error {}", kRegistryKey, result.Code());
        return false;
    }

    result = registry.TrySetDwordValue(kRegistryValue, 1);
    if (!result) {
        spdlog::error(L"FirewallOnBootManager::enable failed writing value {}: error {}", kRegistryValue, result.Code());
        return false;
    }

    spdlog::info("Firewall on boot enabled");
    return true;
}

bool FirewallOnBootManager::disable()
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, kRegistryKey);
    if (!result) {
        if (result.Code() == ERROR_FILE_NOT_FOUND) {
            return true;
        }
        spdlog::error(L"FirewallOnBootManager::disable failed opening key {}: error {}", kRegistryKey, result.Code());
        return false;
    }

    if (registry.TryContainsValue(kRegistryValue)) {
        result = registry.TryDeleteValue(kRegistryValue);
        if (!result) {
            spdlog::error(L"FirewallOnBootManager::disable failed deleting value {}: error {}", kRegistryValue, result.Code());
            return false;
        }

        spdlog::info("Firewall on boot disabled");
    }

    return true;
}
