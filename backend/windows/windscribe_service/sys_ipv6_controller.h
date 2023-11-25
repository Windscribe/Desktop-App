#pragma once

class SysIpv6Controller
{
public:
    static SysIpv6Controller &instance()
    {
        static SysIpv6Controller sic;
        return sic;
    }

    bool isIpv6Enabled();
    void setIpv6Enabled(bool bEnabled);

private:
    SysIpv6Controller() {}
    ~SysIpv6Controller() {}

    LONG getDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD &nValue);
    LONG setDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD nValue);
};

