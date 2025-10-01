#pragma once

#include <Windows.h>
#include <string>

namespace Registry
{

bool regWriteDwordProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, DWORD value);
bool regWriteSzProperty(HKEY h, const wchar_t *subkeyName, const std::wstring& valueName, const std::wstring& value);
bool regDeleteProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName);
bool regGetProperty(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName, LPBYTE value, DWORD* size);
bool regAddDwordValueIfNotExists(HKEY h, const std::wstring& subkeyName, const std::wstring& valueName);

// Returns true if the given parent key contains the subkey.
bool subkeyExists(HKEY rootKey, const std::wstring &parentKey, const std::wstring& subkey);

};
