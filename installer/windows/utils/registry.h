#ifndef REGISTRY_H
#define REGISTRY_H

#include <windows.h>
#include <vector>
#include <string>

#include "reg_view.h"


class Registry
{
 public:
    static bool InternalRegQueryStringValue(HKEY H, const wchar_t *Name, std::wstring &ResultStr, DWORD Type1, DWORD Type2);
    static bool RegQueryStringValue(HKEY H,const wchar_t *SubKeyName,std::wstring ValueName, std::wstring &ResultStr);
    static bool RegQueryStringValue1(HKEY H, const wchar_t *Name, std::wstring &ResultStr);

    static bool RegWriteStringValue(HKEY H, std::wstring SubKeyName, std::wstring ValueName, std::wstring ValueData);
    static bool RegWriteStringValue(HKEY H, std::wstring SubKeyName, std::wstring ValueName, std::string ValueData);

    static LSTATUS RegCreateKeyExView(const TRegView &RegView, HKEY &hKey, LPCWSTR lpSubKey,
                                      DWORD Reserved, LPWSTR lpClass,DWORD dwOptions, REGSAM samDesired,
                                      LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult,
                                      LPDWORD lpdwDisposition);

    static LSTATUS RegOpenKeyExView(TRegView RegView, HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY &phkResult);

    static bool RegDeleteValue1(HKEY H,const wchar_t *SubKeyName,std::wstring ValueName);

    static LSTATUS RegDeleteKeyIncludingSubkeys1(TRegView RegView, const HKEY Key, const wchar_t *Name);
    static bool RegDeleteKeyIncludingSubkeys(HKEY RootKey, const std::wstring SubkeyName);

    static LSTATUS RegDeleteKeyView(TRegView RegView, const HKEY Key,  const wchar_t *Name);
    static std::wstring GetRegRootKeyName(const HKEY RootKey);

    // non-legacy
    static std::vector<std::wstring> regGetSubkeyChildNames(HKEY h, const wchar_t *subkeyName);
    static bool regDeleteProperty(HKEY h, const wchar_t *subkeyName, std::wstring valueName);
    static bool regHasSubkeyProperty(HKEY h, const wchar_t *subkeyName, std::wstring valueName);
};

#endif // REGISTRY_H
