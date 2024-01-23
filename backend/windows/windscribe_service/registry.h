#pragma once

#include <Windows.h>
#include <string>

class Registry
{

 public:
    static bool regWriteDwordProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, DWORD value);
    static bool regWriteSzProperty(HKEY h, const wchar_t *subkeyName, const std::wstring& valueName, const std::wstring& value);
    static bool regDeleteProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName);
    static bool regGetProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, LPBYTE value, DWORD* size);
    static bool regAddDwordValueIfNotExists(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName);
};
