#pragma once

class SysIpv6Controller
{
public:
	bool isIpv6Enabled();
	void setIpv6Enabled(bool bEnabled);

private:
	LONG getDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD &nValue);
	LONG setDWORDRegKey(HKEY hKey, const wchar_t *strValueName, DWORD nValue);
};

