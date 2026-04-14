#include "ws_branding.h"
#include "remove_app_network_profiles.h"

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

namespace RemoveAppNetworkProfiles
{

static bool isAppNetworkProfile(HKEY hKeyParent, const std::wstring &subKey)
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(hKeyParent, subKey);
    if (!result) {
        spdlog::error(L"isAppNetworkProfile failed opening network profiles key {}: error {}", subKey, result.Code());
        return false;
    }

    const auto value = registry.TryGetStringValue(L"Description");
    if (!value) {
        if (value.GetError().Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"isAppNetworkProfile failed reading the 'Description' value for key {}: error {}", subKey, value.GetError().Code());
        }
        return false;
    }

    return (wcsstr(value.GetValue().c_str(), WS_PRODUCT_NAME_W) != NULL);
}

void remove()
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles");
    if (!result) {
        spdlog::error(L"RemoveAppNetworkProfiles failed opening network profiles key: error {}", result.Code());
        return;
    }

    const auto subKeys = registry.TryEnumSubKeys();
    if (!subKeys) {
        spdlog::error(L"RemoveAppNetworkProfiles failed enumerating network profiles keys: error {}", subKeys.GetError().Code());
        return;
    }

    for (const auto &subKey : subKeys.GetValue()) {
        if (isAppNetworkProfile(registry.Get(), subKey)) {
            result = registry.TryDeleteTree(subKey);
            if (!result) {
                spdlog::error(L"RemoveAppNetworkProfiles failed deleting network profiles key {}: error {}", subKey, result.Code());
            }
        }
    }
}

}
