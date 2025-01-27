#include "registry.h"

#include <winreg/WinReg.hpp>

#include <spdlog/spdlog.h>
#include "utils.h"
#include "utils/wsscopeguard.h"

using namespace std;

namespace Registry
{

// TODO **JDRM** (tech debt) refactor these methods to use the winreg class.

bool regWriteDwordProperty(HKEY h, const wstring &subkeyName, const wstring &valueName, DWORD value)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (hKey != NULL)
            RegCloseKey(hKey);
    });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND) {
        nError = RegCreateKeyEx(h, subkeyName.c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error creating/opening registry key {}: {}", subkeyName, nError);
        return false;
    }

    nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error setting registry key {} - {}: {}", subkeyName, valueName, nError);
        return false;
    }
    else {
        spdlog::debug(L"Successfully set registry key {} - {}: {}", subkeyName, valueName, value);
        return true;
    }
}

bool regWriteSzProperty(HKEY h, const wchar_t * subkeyName, const wstring &valueName, const wstring &value)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (hKey != NULL)
            RegCloseKey(hKey);
    });

    LONG nError = RegOpenKeyEx(h, subkeyName, NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND) {
        nError = RegCreateKeyEx(h, subkeyName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error creating/opening registry key {}: {}", subkeyName, nError);
        return false;
    }

    nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (DWORD)((value.length() + 1) * sizeof(wchar_t)));

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error setting registry key {} - {}: {}", subkeyName, valueName, nError);
        return false;
    }
    else {
        spdlog::debug(L"Successfully set registry key {} - {}: {}", subkeyName, valueName, value);
        return true;
    }
}

// todo Function returns true if prop was not removed and false otherwise. Probably it should be changed.
bool regDeleteProperty(HKEY h, const wstring &subkeyName, const wstring &valueName)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (hKey != NULL)
            RegCloseKey(hKey);
    });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), 0, KEY_SET_VALUE, &hKey);

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error opening registry key {}: {}", subkeyName, nError);
        return false;
    }

    nError = RegDeleteValue(hKey, valueName.c_str());

    if (nError == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    else if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error deleting registry key value {} - {}: {}", subkeyName, valueName, nError);
        return false;
    }
    else {
        spdlog::debug(L"Successfully deleted registry key value {} - {}: {}", subkeyName, valueName, nError);
        return true;
    }
}

bool regGetProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, LPBYTE value, DWORD* size)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (hKey != NULL)
            RegCloseKey(hKey);
    });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), 0, KEY_READ, &hKey);

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Error opening registry key {}: {}", subkeyName, nError);
        return false;

    }

    nError = RegQueryValueExW(hKey, valueName.c_str(), 0, NULL, value, size);

    if (nError != ERROR_SUCCESS) {
        spdlog::error(L"Property {} not found for key {}: {}", valueName, subkeyName, nError);
        return false;
    }

    return true;
}

bool regAddDwordValueIfNotExists(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName)
{
    return regWriteDwordProperty(h, subkeyName, valueName, 0);
}

bool subkeyExists(HKEY rootKey, const std::wstring& parentKey, const std::wstring& subkey)
{
    bool exists = false;
    try {
        winreg::RegKey registry;
        registry.Open(rootKey, parentKey);
        const auto subkeys = registry.EnumSubKeys();
        for (const auto &key : subkeys) {
            // Perform a case-insensitive search since registry key paths are case-insensitive.
            if (Utils::iequals(key, subkey)) {
                exists = true;
                break;
            }
        }
    }
    catch (winreg::RegException &ex) {
        spdlog::error(L"subkeyExists error checking key {}", parentKey);
        spdlog::error("subkeyExists error ex.what = {}", ex.what());
    }

    return exists;
}

};
