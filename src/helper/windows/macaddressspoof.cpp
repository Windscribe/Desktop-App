#include "macaddressspoof.h"

#include <spdlog/spdlog.h>
#include <winreg/WinReg.hpp>

#include "utils.h"

namespace MacAddressSpoof
{

static const std::wstring kMacSpoofRegistryKey = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";

static bool subkeyExists(const std::wstring& subkey)
{
    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, kMacSpoofRegistryKey);
    if (!result) {
        spdlog::error(L"MacAddressSpoof::subkeyExists failed opening key {}: error {}", kMacSpoofRegistryKey, result.Code());
        return false;
    }

    const auto subKeys = registry.TryEnumSubKeys();
    if (!subKeys) {
        spdlog::error(L"MacAddressSpoof::subkeyExists failed enumerating subkeys for key {}: error {}", kMacSpoofRegistryKey, subKeys.GetError().Code());
        return false;
    }

    bool exists = false;
    for (const auto &key : subKeys.GetValue()) {
        // Perform a case-insensitive search since registry key paths are case-insensitive.
        if (Utils::iequals(key, subkey)) {
            exists = true;
            break;
        }
    }

    return exists;
}

void set(const std::wstring &interfaceName, const std::wstring &value)
{
    if (!subkeyExists(interfaceName)) {
        spdlog::error(L"MacAddressSpoof::set did not find key {}", interfaceName);
        return;
    }

    if (!Utils::isMacAddress(value)) {
        spdlog::error(L"MacAddressSpoof::set received an invalid MAC address {}", value);
        return;
    }

    const std::wstring subKey = kMacSpoofRegistryKey + L"\\" + interfaceName;

    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, subKey);
    if (!result) {
        spdlog::error(L"MacAddressSpoof::set failed opening key {}: error {}", subKey, result.Code());
        return;
    }

    result = registry.TrySetStringValue(L"NetworkAddress", value);
    if (!result) {
        spdlog::error(L"MacAddressSpoof::set failed setting 'NetworkAddress' to {} for key {}: error {}", value, subKey, result.Code());
        return;
    } else {
        spdlog::debug(L"MacAddressSpoof::set 'NetworkAddress' to {} for key {}", value, subKey);
    }

    result = registry.TrySetDwordValue(L"WindscribeMACSpoofed", 1);
    if (!result) {
        spdlog::error(L"MacAddressSpoof::set failed writing 'WindscribeMACSpoofed' to key {}: error {}", subKey, result.Code());
    }
}

void remove(const std::wstring &interfaceName)
{
    if (!subkeyExists(interfaceName)) {
        spdlog::error(L"MacAddressSpoof::remove did not find key {}", interfaceName);
        return;
    }

    const std::wstring subKey = kMacSpoofRegistryKey + L"\\" + interfaceName;

    winreg::RegKey registry;
    winreg::RegResult result = registry.TryOpen(HKEY_LOCAL_MACHINE, subKey);
    if (!result) {
        spdlog::error(L"MacAddressSpoof::remove failed opening key {}: error {}", subKey, result.Code());
        return;
    }

    result = registry.TryDeleteValue(L"NetworkAddress");
    if (!result) {
        if (result.Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"MacAddressSpoof::remove failed deleting 'NetworkAddress' from key {}: error {}", subKey, result.Code());
        }
    } else {
        spdlog::debug(L"MacAddressSpoof::remove 'NetworkAddress' from key {}", subKey);
    }

    result = registry.TryDeleteValue(L"WindscribeMACSpoofed");
    if (!result) {
        if (result.Code() != ERROR_FILE_NOT_FOUND) {
            spdlog::error(L"MacAddressSpoof::remove failed deleting 'WindscribeMACSpoofed' from key {}: error {}", subKey, result.Code());
        }
    }
}

}
