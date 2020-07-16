#include "all_headers.h"
#include "remove_windscribe_network_profiles.h"

/*#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <strsafe.h>*/

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

void RemoveWindscribeNetworkProfiles::remove()
{
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles", 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		enumAll(hKey);
		RegCloseKey(hKey);
	}
}

void RemoveWindscribeNetworkProfiles::enumAll(HKEY hKey)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	DWORD    cntSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 

	DWORD i, retCode;

	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		NULL,                // buffer for class name 
		0,           // size of class string 
		NULL,                    // reserved 
		&cntSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		NULL,   // security descriptor 
		NULL);       // last write time 

	// Enumerate the subkeys, until RegEnumKeyEx fails.
	std::vector<std::wstring> keysForRemove;

	if (cntSubKeys)
	{
		for (i = 0; i < cntSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, NULL);
			if (retCode == ERROR_SUCCESS)
			{
				if (isNeedRemove(achKey))
				{
					keysForRemove.push_back(achKey);
				}
			}
		}
	}

	for (auto it = keysForRemove.begin(); it != keysForRemove.end(); ++it)
	{
		regDelnodeRecurse(hKey, (LPTSTR)it->c_str());
	}
}

bool RemoveWindscribeNetworkProfiles::isNeedRemove(const wchar_t *name)
{
	bool bRet = false;
	const int maxStrLen = 1024;
	HKEY hKey;
	std::wstring szPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles\\";
	szPath += name;

	LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath.c_str(), 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		DWORD dataSize;
		DWORD type;
		if (RegQueryValueEx(hKey, L"Description", NULL, &type, NULL, &dataSize) == ERROR_SUCCESS)
		{
			if (type == REG_SZ && dataSize < maxStrLen)
			{
				wchar_t str[maxStrLen];
				if (RegQueryValueEx(hKey, L"Description", NULL, NULL, (LPBYTE)str, &dataSize) == ERROR_SUCCESS)
				{
					if (wcsstr(str, L"Windscribe") != NULL)
					{
						bRet = true;
					}
				}
			}
		}
		RegCloseKey(hKey);
	}

	return bRet;
}


bool RemoveWindscribeNetworkProfiles::regDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having
	// to recurse.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
	{
		wprintf(TEXT("removed %s\n"), lpSubKey);
		return TRUE;
	}

	lResult = RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS)
	{
		if (lResult == ERROR_FILE_NOT_FOUND) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.

	lpEnd = lpSubKey + lstrlen(lpSubKey);

	if (*(lpEnd - 1) != TEXT('\\'))
	{
		*lpEnd = TEXT('\\');
		lpEnd++;
		*lpEnd = TEXT('\0');
	}

	// Enumerate the keys

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
		NULL, NULL, &ftWrite);

	if (lResult == ERROR_SUCCESS)
	{
		do {

			*lpEnd = TEXT('\0');
			StringCchCat(lpSubKey, MAX_PATH * 2, szName);

			if (!regDelnodeRecurse(hKeyRoot, lpSubKey)) {
				break;
			}

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
				NULL, NULL, &ftWrite);

		} while (lResult == ERROR_SUCCESS);
	}

	lpEnd--;
	*lpEnd = TEXT('\0');

	RegCloseKey(hKey);

	// Try again to delete the key.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS)
		return TRUE;

	return FALSE;
}