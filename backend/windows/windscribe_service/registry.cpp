#include "all_headers.h"
#include "registry.h"
#include "logger.h"

using namespace std;

bool Registry::regWriteDwordProperty(HKEY h, std::wstring subkeyName, std::wstring valueName, DWORD value)
{
	HKEY hKey;
	LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), NULL, KEY_ALL_ACCESS, &hKey);

	if (nError == ERROR_FILE_NOT_FOUND)
	{
		nError = RegCreateKeyEx(h, subkeyName.c_str(), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	}

	if (nError != ERROR_SUCCESS)
	{
		Logger::instance().out("Error creating/opening registry key");
	}
	else
	{
		nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
		if (nError != ERROR_SUCCESS)
		{
			std::wstring output = L"Error setting registry key " + subkeyName + L" - " + valueName + L": " + std::to_wstring(nError);
			Logger::instance().out(output.c_str());

		}
		else
		{
			std::wstring output = L"Successfully set registry key " + subkeyName + L" - " + valueName + L": " + std::to_wstring(value);
			Logger::instance().out(output.c_str());
		}
		RegCloseKey(hKey);
	}
	return false;
}

bool Registry::regWriteSzProperty(HKEY h, const wchar_t * subkeyName, std::wstring valueName, std::wstring value)
{

	bool result = false;
	HKEY hKey;
	LONG nError = RegOpenKeyEx(h, subkeyName, NULL, KEY_ALL_ACCESS, &hKey);

	if (nError == ERROR_FILE_NOT_FOUND)
	{
		nError = RegCreateKeyEx(h, subkeyName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	}

	if (nError != ERROR_SUCCESS)
	{
		std::wstring output = L"Error creating/opening registry key";
		Logger::instance().out(output.c_str());
	}
	else
	{
		nError = RegSetValueEx(hKey, valueName.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), (DWORD)((value.length() + 1) * sizeof(wchar_t)));
		
		if (nError != ERROR_SUCCESS)
		{
			std::wstring output = L"Error setting registry key " + std::wstring(subkeyName) + L" - " + valueName + L": " + std::to_wstring(nError);
			Logger::instance().out(output.c_str());
		}
		else
		{
			std::wstring output = L"Successfully set registry key " + std::wstring(subkeyName) + L" - " + valueName + L": " + value;
			Logger::instance().out(output.c_str());
			result = true;
		}
		RegCloseKey(hKey);
	}
	return result;
}

bool Registry::regDeleteProperty(HKEY h, std::wstring subkeyName, std::wstring valueName)
{
	bool ret = true;
	HKEY K;

	LONG nError = RegOpenKeyEx(h, subkeyName.c_str(), 0, KEY_SET_VALUE, &K);

	if (nError == ERROR_SUCCESS)
	{
		if (RegDeleteValue(K, valueName.c_str()) == ERROR_SUCCESS)
		{
			ret = false;
		}

		RegCloseKey(K);
	}
	else
	{
		ret = false;
	}

	return ret;
}





