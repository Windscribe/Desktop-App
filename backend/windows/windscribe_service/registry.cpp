#include "registry.h"
#include "logger.h"

#include "../../../client/common/utils/wsscopeguard.h"

using namespace std;

bool Registry::regWriteDwordProperty(HKEY h, const wstring &subkeyName, const wstring &valueName, DWORD value)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&]
                                       {
                                           if (hKey != NULL)
                                               RegCloseKey(hKey);
                                       });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND) {
        nError = RegCreateKeyEx(h, subkeyName.c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error creating/opening registry key %s: %d", subkeyName.c_str(), nError);
        return false;
    }

    nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error setting registry key %s - %s: %ld", subkeyName.c_str(), valueName.c_str(), nError);
        return false;
    }
    else {
        Logger::instance().out(L"Successfully set registry key %s - %s: %lu", subkeyName.c_str(), valueName.c_str(), value);
        return true;
    }
}

bool Registry::regWriteSzProperty(HKEY h, const wchar_t * subkeyName, const wstring &valueName, const wstring &value)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&]
                                       {
                                           if (hKey != NULL)
                                               RegCloseKey(hKey);
                                       });

    LONG nError = RegOpenKeyEx(h, subkeyName, NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND) {
        nError = RegCreateKeyEx(h, subkeyName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error creating/opening registry key %s: %d", subkeyName, nError);
        return false;
    }

    nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (DWORD)((value.length() + 1) * sizeof(wchar_t)));

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error setting registry key %s - %s: %ld", subkeyName, valueName.c_str(), nError);
        return false;
    }
    else {
        Logger::instance().out(L"Successfully set registry key %s - %s: %s", subkeyName, valueName.c_str(), value.c_str());
        return true;
    }
}

// todo Function returns true if prop was not removed and false otherwise. Probably it should be changed.
bool Registry::regDeleteProperty(HKEY h, const wstring &subkeyName, const wstring &valueName)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&]
                                       {
                                           if (hKey != NULL)
                                               RegCloseKey(hKey);
                                       });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), 0, KEY_SET_VALUE, &hKey);

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error opening registry key %s: %d", subkeyName.c_str(), nError);
        return false;
    }

    nError = RegDeleteValue(hKey, valueName.c_str());

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error deleting registry key value %s - %s: %ld", subkeyName.c_str(), valueName.c_str(), nError);
        return false;
    }
    else {
        Logger::instance().out(L"Successfully deleted registry key value %s - %s: %ld", subkeyName.c_str(), valueName.c_str(), nError);
        return true;
    }
}

bool Registry::regGetProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, LPBYTE value, DWORD* size)
{
    HKEY hKey{ NULL };
    auto exitGuard = wsl::wsScopeGuard([&]
                                       {
                                           if (hKey != NULL)
                                               RegCloseKey(hKey);
                                       });

    LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), 0, KEY_READ, &hKey);

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Error opening registry key %s: %d", subkeyName.c_str(), nError);
        return false;

    }

    nError = RegQueryValueExW(hKey, valueName.c_str(), 0, NULL, value, size);

    if (nError != ERROR_SUCCESS) {
        Logger::instance().out(L"Property %s not found for key %s: %d", valueName.c_str(), subkeyName.c_str(), nError);
        return false;
    }

    return true;
}

bool Registry::regAddDwordValueIfNotExists(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName)
{
    return regWriteDwordProperty(h, subkeyName, valueName, 0);
}
