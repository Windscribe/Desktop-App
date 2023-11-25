#pragma once

#include <windows.h>
#include <vector>
#include <string>

class Registry
{
 public:
    static bool InternalRegQueryStringValue(HKEY H, const wchar_t *Name, std::wstring &ResultStr, DWORD Type1, DWORD Type2);
    static bool RegQueryStringValue(HKEY H, const wchar_t *SubKeyName, std::wstring ValueName, std::wstring &ResultStr);
    static bool RegQueryStringValue1(HKEY H, const wchar_t *Name, std::wstring &ResultStr);

    static LSTATUS RegOpenKeyExView(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY &phkResult);

    static LSTATUS RegDeleteKeyIncludingSubkeys1(const HKEY Key, const wchar_t *Name);

    static LSTATUS RegDeleteKeyView(const HKEY Key,  const wchar_t *Name);

    // non-legacy
    static std::vector<std::wstring> regGetSubkeyChildNames(HKEY h, const wchar_t *subkeyName);
    static bool regDeleteProperty(HKEY h, const wchar_t *subkeyName, std::wstring valueName);
    static bool regHasSubkeyProperty(HKEY h, const wchar_t *subkeyName, std::wstring valueName);
};
