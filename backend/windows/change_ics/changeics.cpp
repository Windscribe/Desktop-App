// ChangeIcs.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <rpc.h>
#include <NetCon.h>
#include <vector>
#include <map>
#include "file_wrapper.h"
#include "miniatl.h"

#pragma comment(lib, "rpcrt4.lib")

typedef void (WINAPI *PFNNcFreeNetconProperties)(NETCON_PROPERTIES* pProps);
PFNNcFreeNetconProperties ncFreeNetconProperties = NULL;

void disableIcsOnAll(INetSharingManager *pNSM, std::vector<CComPtr<INetConnection> > &pNetConnections)
{
	HRESULT hr;
	for (size_t i = 0; i < pNetConnections.size(); ++i)
	{
		CComPtr<INetSharingConfiguration> pNetConfiguration;
		hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnections[i], &pNetConfiguration);
		if (hr == S_OK)
		{
			VARIANT_BOOL bEnabled = VARIANT_FALSE;
			hr = pNetConfiguration->get_SharingEnabled(&bEnabled);
			if (hr == S_OK)
			{
				if (bEnabled == VARIANT_TRUE)
				{
					pNetConfiguration->DisableSharing();
				}
			}
		}
	}
}

bool changeImpl(const GUID &publicGuid, const GUID &privateGuid)
{
	CComPtr<INetSharingManager> pNSM;

	HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL,
		__uuidof(INetSharingManager), (void**)&pNSM);

	if (hr != S_OK)
	{
		printf("CoCreateInstance NetSharingManager failed\n");
		return false;
	}

	CComPtr<INetSharingEveryConnectionCollection> pConnectionsList;
	hr = pNSM->get_EnumEveryConnection(&pConnectionsList);
	if (hr != S_OK)
	{
		printf("get_EnumEveryConnection failed, %ld\n", hr);
		return false;
	}

	CComPtr<IUnknown> pUnkEnum;
	hr = pConnectionsList->get__NewEnum(&pUnkEnum);
	if (hr != S_OK)
	{
		printf("get__NewEnum failed, %ld\n", hr);
		return false;
	}

	CComPtr<IEnumNetSharingEveryConnection> pNSEConn;
	hr = pUnkEnum->QueryInterface(__uuidof(IEnumNetSharingEveryConnection), (void**)&pNSEConn);
	if (hr != S_OK)
	{
		printf("QueryInterface IEnumNetSharingEveryConnection failed, %ld\n", hr);
		return false;
	}

	std::vector< CComPtr<INetConnection> > pNetConnections;
	CComPtr<INetConnection> pNetConnection;
	CComPtr<INetConnection> pNetConnectionPublic;
	CComPtr<INetConnection> pNetConnectionPrivate;
	VARIANT varItem;

	while (1)
	{
		pNetConnection = NULL;
		VariantClear(&varItem);

		hr = pNSEConn->Next(1, &varItem, NULL);
		if (S_FALSE == hr)
		{
			hr = S_OK;
			break;
		}

		if ((V_VT(&varItem) == VT_UNKNOWN) && V_UNKNOWN(&varItem))
		{
			hr = V_UNKNOWN(&varItem)->QueryInterface(__uuidof(INetConnection),
				(void**)&pNetConnection);
			if (hr == S_OK)
			{
				pNetConnections.push_back(pNetConnection);

				NETCON_PROPERTIES *ppProps = NULL;
				hr = pNetConnection->GetProperties(&ppProps);
				if (hr == S_OK)
				{
					if (ppProps->guidId == privateGuid)
					{
						pNetConnectionPrivate = pNetConnection;
					}
					else if (ppProps->guidId == publicGuid)
					{
						pNetConnectionPublic = pNetConnection;
					}

					if (ncFreeNetconProperties)
					{
						ncFreeNetconProperties(ppProps);
					}
				}
			}
		}
	}

	if (pNetConnectionPublic == NULL)
	{
		printf("Not found public network connection by guid\n");
		return false;
	}
	if (pNetConnectionPrivate == NULL)
	{
		printf("Not found private network connection by guid\n");
		return false;
	}

	// get configuration interfaces
	CComPtr<INetSharingConfiguration> pNetConfigurationPublic;
	CComPtr<INetSharingConfiguration> pNetConfigurationPrivate;

	hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnectionPublic, &pNetConfigurationPublic);
	if (hr != S_OK)
	{
		printf("get_INetSharingConfigurationForINetConnection failed, %ld\n", hr);
		return false;
	}
	hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnectionPrivate, &pNetConfigurationPrivate);
	if (hr != S_OK)
	{
		printf("get_INetSharingConfigurationForINetConnection failed, %ld\n", hr);
		return false;
	}

	// Check if the ICS on public/private interfaces are already enabled.
	// if so, no need to start ICS again.
	VARIANT_BOOL bPublicEnabled = VARIANT_FALSE;
	SHARINGCONNECTIONTYPE typePublic = ICSSHARINGTYPE_PUBLIC;
	hr = pNetConfigurationPublic->get_SharingEnabled(&bPublicEnabled);
	if (hr != S_OK)
	{
		printf("get_SharingEnabled failed for public guid, %ld\n", hr);
		return false;
	}
	hr = pNetConfigurationPublic->get_SharingConnectionType(&typePublic);
	if (hr != S_OK)
	{
		printf("get_SharingConnectionType failed for public guid, %ld\n", hr);
		return false;
	}

	VARIANT_BOOL bPrivateEnabled = VARIANT_FALSE;
	SHARINGCONNECTIONTYPE typePrivate = ICSSHARINGTYPE_PUBLIC;
	hr = pNetConfigurationPrivate->get_SharingEnabled(&bPrivateEnabled);
	if (hr != S_OK)
	{
		printf("get_SharingEnabled failed for private guid, %ld\n", hr);
		return false;
	}
	hr = pNetConfigurationPrivate->get_SharingConnectionType(&typePrivate);
	if (hr != S_OK)
	{
		printf("get_SharingConnectionType failed for private guid, %ld\n", hr);
		return false;
	}

	if (bPublicEnabled == VARIANT_TRUE && typePublic == ICSSHARINGTYPE_PUBLIC &&
		bPrivateEnabled == VARIANT_TRUE && typePrivate == ICSSHARINGTYPE_PRIVATE)
	{
		printf("nothing need to change in ics manager\n");
		return true;
	}

	// Disable existing ICS first
	disableIcsOnAll(pNSM, pNetConnections);

	DWORD dw = pNetConfigurationPublic->EnableSharing(ICSSHARINGTYPE_PUBLIC);
	if (dw != S_OK)
	{
		printf("EnableSharing failed for public guid, %ld\n", dw);
		return false;
	}

	hr = pNetConfigurationPrivate->EnableSharing(ICSSHARINGTYPE_PRIVATE);
	if (hr != S_OK)
	{
		printf("EnableSharing failed for private guid, %ld\n", dw);
		return false;
	}

	return true;
}

bool saveConnection(INetSharingManager *pNSM, std::vector<SHARING_OPTIONS> &vectorSaved, const GUID &guidId, CComPtr<INetConnection> &pNetConnection)
{
	CComPtr<INetSharingConfiguration> pNetConfiguration;

	HRESULT hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnection, &pNetConfiguration);
	if (hr != S_OK)
	{
		return false;
	}

	VARIANT_BOOL bSharingEnabled = VARIANT_FALSE;
	SHARINGCONNECTIONTYPE typeOfSharing = ICSSHARINGTYPE_PUBLIC;
	hr = pNetConfiguration->get_SharingEnabled(&bSharingEnabled);
	if (hr != S_OK)
	{
		return false;
	}
	hr = pNetConfiguration->get_SharingConnectionType(&typeOfSharing);
	if (hr != S_OK)
	{
		return false;
	}

	SHARING_OPTIONS opt;
	opt.guid = guidId;
	opt.bSharingEnabled = bSharingEnabled;
	opt.typeOfSharing = typeOfSharing;
	vectorSaved.push_back(opt);

	return true;
}

bool saveIcsSettings(wchar_t *szFilePath)
{
	FileWrapper file;
	std::vector<SHARING_OPTIONS> vectorSaved;

	if (!file.open(szFilePath, L"wb"))
	{
		wprintf(L"Can't create file: %s\n", szFilePath);
		return false;
	}

	CComPtr<INetSharingManager> pNSM;

	HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL,
		__uuidof(INetSharingManager), (void**)&pNSM);

	if (hr != S_OK)
	{
		printf("CoCreateInstance NetSharingManager failed\n");
		return false;
	}

	CComPtr<INetSharingEveryConnectionCollection> pConnectionsList;
	hr = pNSM->get_EnumEveryConnection(&pConnectionsList);
	if (hr != S_OK)
	{
		printf("get_EnumEveryConnection failed in IcsManager::saveIcsSettings, %ld\n", hr);
		return false;
	}

	CComPtr<IUnknown> pUnkEnum;
	hr = pConnectionsList->get__NewEnum(&pUnkEnum);
	if (hr != S_OK)
	{
		printf("get__NewEnum failed in IcsManager::saveIcsSettings, %ld\n", hr);
		return false;
	}

	CComPtr<IEnumNetSharingEveryConnection> pNSEConn;
	hr = pUnkEnum->QueryInterface(__uuidof(IEnumNetSharingEveryConnection), (void**)&pNSEConn);
	if (hr != S_OK)
	{
		printf("QueryInterface IEnumNetSharingEveryConnection failed in IcsManager::saveIcsSettings, %ld\n", hr);
		return false;
	}

	VARIANT varItem;
	while (1)
	{
		VariantClear(&varItem);

		hr = pNSEConn->Next(1, &varItem, NULL);
		if (S_FALSE == hr)
		{
			hr = S_OK;
			break;
		}

		if ((V_VT(&varItem) == VT_UNKNOWN) && V_UNKNOWN(&varItem))
		{
			CComPtr<INetConnection> pNetConnection;
			hr = V_UNKNOWN(&varItem)->QueryInterface(__uuidof(INetConnection),
				(void**)&pNetConnection);
			if (hr == S_OK)
			{
				NETCON_PROPERTIES *ppProps = NULL;
				hr = pNetConnection->GetProperties(&ppProps);
				if (hr == S_OK)
				{
					if (!saveConnection(pNSM, vectorSaved, ppProps->guidId, pNetConnection))
					{
						wprintf(L"saveConnection failed in IcsManager::saveIcsSettings, %s\n", ppProps->pszwDeviceName);
					}

					if (ncFreeNetconProperties)
					{
						ncFreeNetconProperties(ppProps);
					}
				}
			}
		}
	}

	if (!file.write(vectorSaved))
	{
		wprintf(L"Can't write to file: %s\n", szFilePath);
		return false;
	}

	return true;
}

bool restoreConnectionIfNeed(INetSharingManager *pNSM, std::vector<SHARING_OPTIONS> &vectorSaved, const GUID &guidId, CComPtr<INetConnection> &pNetConnection)
{
	size_t ind;
	bool bFound = false;

	for (size_t i = 0; i < vectorSaved.size(); ++i)
	{
		if (vectorSaved[i].guid == guidId)
		{
			ind = i;
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		return true;
	}

	CComPtr<INetSharingConfiguration> pNetConfiguration;
	HRESULT hr = pNSM->get_INetSharingConfigurationForINetConnection(pNetConnection, &pNetConfiguration);
	if (hr != S_OK)
	{
		return false;
	}

	VARIANT_BOOL bSharingEnabled = VARIANT_FALSE;
	SHARINGCONNECTIONTYPE typeOfSharing = ICSSHARINGTYPE_PUBLIC;
	hr = pNetConfiguration->get_SharingEnabled(&bSharingEnabled);
	if (hr != S_OK)
	{
		return false;
	}
	hr = pNetConfiguration->get_SharingConnectionType(&typeOfSharing);
	if (hr != S_OK)
	{
		return false;
	}

	if (bSharingEnabled != vectorSaved[ind].bSharingEnabled || typeOfSharing != vectorSaved[ind].typeOfSharing)
	{
		if (vectorSaved[ind].bSharingEnabled == VARIANT_TRUE)
		{
			hr = pNetConfiguration->EnableSharing(vectorSaved[ind].typeOfSharing);
			if (hr != S_OK)
			{
				printf("EnableSharing failed, %ld\n", hr);
				return false;
			}

		}
		else
		{
			hr = pNetConfiguration->DisableSharing();
			if (hr != S_OK)
			{
				printf("DisableSharing failed, %ld\n", hr);
				return false;
			}

		}
	}

	return true;
}

bool restoreIcsSettings(wchar_t *szFilePath)
{
	FileWrapper file;
	std::vector<SHARING_OPTIONS> vectorSaved;

	if (!file.open(szFilePath, L"rb"))
	{
		wprintf(L"Can't open file: %s\n", szFilePath);
		return false;
	}

	if (!file.read(vectorSaved))
	{
		wprintf(L"Can't read file: %s\n", szFilePath);
		return false;
	}

	CComPtr<INetSharingManager> pNSM;

	HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL,
		__uuidof(INetSharingManager), (void**)&pNSM);

	if (hr != S_OK)
	{
		printf("CoCreateInstance NetSharingManager failed\n");
		return false;
	}

	CComPtr<INetSharingEveryConnectionCollection> pConnectionsList;
	hr = pNSM->get_EnumEveryConnection(&pConnectionsList);
	if (hr != S_OK)
	{
		printf("get_EnumEveryConnection failed in IcsManager::restoreIcsSettings, %ld\n", hr);
		return false;
	}

	CComPtr<IUnknown> pUnkEnum;
	hr = pConnectionsList->get__NewEnum(&pUnkEnum);
	if (hr != S_OK)
	{
		printf("get__NewEnum failed in IcsManager::restoreIcsSettings, %ld\n", hr);
		return false;
	}

	CComPtr<IEnumNetSharingEveryConnection> pNSEConn;
	hr = pUnkEnum->QueryInterface(__uuidof(IEnumNetSharingEveryConnection), (void**)&pNSEConn);
	if (hr != S_OK)
	{
		printf("QueryInterface IEnumNetSharingEveryConnection failed in IcsManager::restoreIcsSettings, %ld\n", hr);
		return false;
	}

	VARIANT varItem;
	while (1)
	{
		VariantClear(&varItem);

		hr = pNSEConn->Next(1, &varItem, NULL);
		if (S_FALSE == hr)
		{
			hr = S_OK;
			break;
		}

		if ((V_VT(&varItem) == VT_UNKNOWN) && V_UNKNOWN(&varItem))
		{
			CComPtr<INetConnection> pNetConnection;
			hr = V_UNKNOWN(&varItem)->QueryInterface(__uuidof(INetConnection),
				(void**)&pNetConnection);
			if (hr == S_OK)
			{
				NETCON_PROPERTIES *ppProps = NULL;
				hr = pNetConnection->GetProperties(&ppProps);
				if (hr == S_OK)
				{
					if (!restoreConnectionIfNeed(pNSM, vectorSaved, ppProps->guidId, pNetConnection))
					{
						wprintf(L"restoreConnectionIfNeed failed in IcsManager::restoreIcsSettings, %s\n", ppProps->pszwDeviceName);
					}

					if (ncFreeNetconProperties)
					{
						ncFreeNetconProperties(ppProps);
					}
				}
			}
		}
	}

	return true;
}


int _tmain(int argc, _TCHAR* argv[])
{
	// 3 commands
	// -change GUID1 GUID2
	// -save filepath
	// -restore filepath

	HMODULE hDLL = LoadLibrary(L"netshell.dll");
	if (hDLL)
	{
		ncFreeNetconProperties = (PFNNcFreeNetconProperties)GetProcAddress(hDLL, "NcFreeNetconProperties");
	}

	if (argc == 3)
	{
		if (wcscmp(argv[1], L"-save") == 0)
		{
			CoInitializeEx(0, COINIT_MULTITHREADED);
			if (saveIcsSettings(argv[2]))
			{
				printf("ok\n");
			}
			else
			{
				printf("failed\n");
			}
			CoUninitialize();
		}
		else if (wcscmp(argv[1], L"-restore") == 0)
		{
			CoInitializeEx(0, COINIT_MULTITHREADED);
			if (restoreIcsSettings(argv[2]))
			{
				printf("ok\n");
			}
			else
			{
				printf("failed\n");
			}
			CoUninitialize();
		}
		else
		{
			return 0;
		}
	}
	else if (argc == 4)
	{
		if (wcscmp(argv[1], L"-change") != 0)
		{
			return 0;
		}

		GUID publicGuid, privateGuid;
		if (UuidFromString((RPC_WSTR)argv[2], &publicGuid) != RPC_S_OK)
		{
			printf("incorrect public guid\n");
			return 0;
		}
		if (UuidFromString((RPC_WSTR)argv[3], &privateGuid) != RPC_S_OK)
		{
			printf("incorrect private guid\n");
			return 0;
		}

		CoInitializeEx(0, COINIT_MULTITHREADED);

		if (changeImpl(publicGuid, privateGuid))
		{
			printf("ok\n");
		}
		else
		{
			printf("failed\n");
		}

		CoUninitialize();
	}

	fflush(stdout);

	return 0;
}

