#pragma once

class RemoveWindscribeNetworkProfiles
{
public:
	static void remove();

private:
	static void enumAll(HKEY hKey);
	static bool isNeedRemove(const wchar_t *name);
	static bool regDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey);
};

