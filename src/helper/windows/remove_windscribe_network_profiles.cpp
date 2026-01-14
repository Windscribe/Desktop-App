#include "remove_windscribe_network_profiles.h"

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

namespace RemoveWindscribeNetworkProfiles
{

static bool isWindscribeNetworkProfile(HKEY hKeyParent, const std::wstring &subKey)
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(hKeyParent, subKey);
    if (!result) {
        spdlog::error(L"isWindscribeNetworkProfile failed opening network profiles key {}: error {}", subKey, result.Code());
        return false;
    }

    const auto value = registry.TryGetStringValue(L"Description");
    if (!value) {
        if (value.GetError().Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"isWindscribeNetworkProfile failed reading the 'Description' value for key {}: error {}", subKey, value.GetError().Code());
        }
        return false;
    }

    return (wcsstr(value.GetValue().c_str(), L"Windscribe") != NULL);
}

void remove()
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles");
    if (!result) {
        spdlog::error(L"RemoveWindscribeNetworkProfiles failed opening network profiles key: error {}", result.Code());
        return;
    }

    const auto subKeys = registry.TryEnumSubKeys();
    if (!subKeys) {
        spdlog::error(L"RemoveWindscribeNetworkProfiles failed enumerating network profiles keys: error {}", subKeys.GetError().Code());
        return;
    }

    for (const auto &subKey : subKeys.GetValue()) {
        if (isWindscribeNetworkProfile(registry.Get(), subKey)) {
            result = registry.TryDeleteTree(subKey);
            if (!result) {
                spdlog::error(L"RemoveWindscribeNetworkProfiles failed deleting network profiles key {}: error {}", subKey, result.Code());
            }
        }
    }
}

}
