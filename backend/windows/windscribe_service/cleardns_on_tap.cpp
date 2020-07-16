#include "all_headers.h"
#include "cleardns_on_tap.h"

void ClearDnsOnTap::clearDns()
{
	ULONG ulAdapterInfoSize = sizeof(IP_ADAPTER_INFO);
	std::vector<unsigned char> pAdapterInfo(ulAdapterInfoSize);

	if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_BUFFER_OVERFLOW) // out of buff
	{
		pAdapterInfo.resize(ulAdapterInfoSize);
	}

	if (GetAdaptersInfo((IP_ADAPTER_INFO *)&pAdapterInfo[0], &ulAdapterInfoSize) == ERROR_SUCCESS)
	{
		IP_ADAPTER_INFO *ai = (IP_ADAPTER_INFO *)&pAdapterInfo[0];

		do
		{
			if (strstr(ai->Description, "Windscribe VPN") != 0)
			{
				regClearDNS(ai->AdapterName);
			}
			ai = ai->Next;
		} while (ai);
	}
}


bool ClearDnsOnTap::regClearDNS(const char *lpszAdapterName)
{
	bool bRet = false;

	std::string strKeyNameWithoutRoot = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + std::string(lpszAdapterName);
	
	HKEY hKey;
	LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, strKeyNameWithoutRoot.c_str(), 0, KEY_ALL_ACCESS, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		std::vector<char> szBuffer;
		DWORD dwBufferSize = 0;

		ULONG nError = RegQueryValueExW(hKey, L"NameServer", 0, NULL, NULL, &dwBufferSize);
		if (nError == ERROR_SUCCESS)
		{
			if (dwBufferSize == 0)
			{
				return true;
			}
			else
			{
				szBuffer.resize(dwBufferSize);
				nError = RegQueryValueExW(hKey, L"NameServer", 0, NULL, (LPBYTE)&szBuffer[0], &dwBufferSize);
				if (nError == ERROR_SUCCESS)
				{
					std::wstring strNameServer((wchar_t *)&szBuffer[0]);
					if (strNameServer.length() != 0)
					{
						nError = RegSetKeyValue(hKey, NULL, L"NameServer", REG_SZ, "", 1);
						bRet = nError == ERROR_SUCCESS;
					}
					else
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
