#include "all_headers.h"
#include "IcsManager.h"


IcsManager::IcsManager()
{
	
}

IcsManager::~IcsManager()
{
}

bool IcsManager::isSupported()
{
	INetSharingManager *pNSM;
	HRESULT hr = ::CoCreateInstance(__uuidof(NetSharingManager), NULL, CLSCTX_ALL, __uuidof(INetSharingManager), (void**)&pNSM);
	if (hr != S_OK)
	{
		return false;
	}

	VARIANT_BOOL bInstalled = VARIANT_FALSE;
	hr = pNSM->get_SharingInstalled(&bInstalled);
	pNSM->Release();
	if (hr != S_OK)
	{
		return false;
	}

	return (bInstalled == VARIANT_TRUE);
}