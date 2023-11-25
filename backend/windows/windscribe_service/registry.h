#pragma once

class Registry
{

 public:
    static bool regWriteDwordProperty(HKEY h, std::wstring subkeyName, std::wstring valueName, DWORD value);
    static bool regWriteSzProperty(HKEY h, const wchar_t *subkeyName, std::wstring valueName, std::wstring value);
    static bool regDeleteProperty(HKEY h, std::wstring subkeyName, std::wstring valueName);

};
