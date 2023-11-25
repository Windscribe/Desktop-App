#include "all_headers.h"
#include "sys_ipv6_controller.h"

bool SysIpv6Controller::isIpv6Enabled()
{
    bool bRet = false;
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters", 0, KEY_READ, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        DWORD value;
        lRes = getDWORDRegKey(hKey, L"DisabledComponents", value);
        if (lRes == ERROR_SUCCESS)
        {
            bRet = (value != 0xFF);
        }
        else
        {
            bRet = true;
        }
        RegCloseKey(hKey);
    }

    return bRet;
}

void SysIpv6Controller::setIpv6Enabled(bool bEnabled)
{
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters", 0, KEY_ALL_ACCESS, &hKey);
    if (lRes == ERROR_SUCCESS)
    {
        setDWORDRegKey(hKey, L"DisabledComponents", bEnabled ? 0x00 : 0xFF);
        RegCloseKey(hKey);
    }
}

LONG SysIpv6Controller::getDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD &nValue)
{
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);
    LONG nError = ::RegQueryValueExW(hKey, strValueName, 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        nValue = nResult;
    }
    return nError;
}

LONG SysIpv6Controller::setDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD nValue)
{
    LONG nError = RegSetValueEx(hKey, strValueName, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&nValue), sizeof(DWORD));
    return nError;
}
