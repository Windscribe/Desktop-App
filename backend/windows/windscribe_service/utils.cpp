#include "all_headers.h"
#include "utils.h"
#include "logger.h"
#include <comdef.h>
#include <WbemIdl.h>
#include "../../../common/utils/executable_signature/executable_signature.h"
#include "../../../common/utils/win32handle.h"

#pragma comment(lib, "wbemuuid.lib")

namespace Utils
{

bool deleteSublayerAndAllFilters(HANDLE engineHandle, const GUID *subLayerGUID)
{
	FWPM_SUBLAYER0 *subLayer;

	DWORD dwRet = FwpmSubLayerGetByKey0(engineHandle, subLayerGUID, &subLayer);
	if (dwRet != ERROR_SUCCESS)
	{
		return false;
	}

	deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_CONNECT_V4);
	deleteAllFiltersForSublayer(engineHandle, subLayerGUID, FWPM_LAYER_ALE_AUTH_CONNECT_V6);

	dwRet = FwpmSubLayerDeleteByKey0(engineHandle, subLayerGUID);
	FwpmFreeMemory0((void **)&subLayer);

	return true;
}

bool deleteAllFiltersForSublayer(HANDLE engineHandle, const GUID *subLayerGUID, GUID layerKey)
{
	FWPM_FILTER_ENUM_TEMPLATE0 enumTemplate;
	ZeroMemory(&enumTemplate, sizeof(enumTemplate));
	enumTemplate.layerKey = layerKey;
	enumTemplate.actionMask = 0xFFFFFFFF;
	HANDLE enumHandle;
	DWORD dwRet = FwpmFilterCreateEnumHandle0(engineHandle, &enumTemplate, &enumHandle);
	if (dwRet != ERROR_SUCCESS)
	{
		return false;
	}

	FWPM_FILTER0 **filters;
	UINT32 numFiltersReturned;
	dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
	while (dwRet == ERROR_SUCCESS && numFiltersReturned > 0)
	{
		for (UINT32 i = 0; i < numFiltersReturned; ++i)
		{
			if (filters[i]->subLayerKey == *subLayerGUID)
			{
				FwpmFilterDeleteById0(engineHandle, filters[i]->filterId);
			}
		}

		FwpmFreeMemory0((void **)&filters);
		dwRet = FwpmFilterEnum0(engineHandle, enumHandle, INFINITE, &filters, &numFiltersReturned);
	}
	dwRet = FwpmFilterDestroyEnumHandle0(engineHandle, enumHandle);
	return true;
}
void addPermitFilterIPv6ForAdapter(NET_IFINDEX tapInd, UINT8 weight, wchar_t *layerName, GUID &subLayerGUID, HANDLE hEngine)
{
	NET_LUID luid;
	ConvertInterfaceIndexToLuid(tapInd, &luid);

	// add permit filter IPv6 for TAP.
	{
		DWORD dwFwAPiRetCode;
		FWPM_FILTER0 filter = { 0 };
		std::vector<FWPM_FILTER_CONDITION0> condition(1);
		memset(&condition[0], 0, sizeof(FWPM_FILTER_CONDITION0) * (1));

		filter.subLayerKey = subLayerGUID;
		filter.displayData.name = layerName;
		filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V6;
		filter.action.type = FWP_ACTION_PERMIT;
		filter.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
		filter.weight.type = FWP_UINT8;
		filter.weight.uint8 = weight;
		filter.filterCondition = &condition[0];
		filter.numFilterConditions = 1;

		condition[0].fieldKey = FWPM_CONDITION_IP_LOCAL_INTERFACE;
		condition[0].matchType = FWP_MATCH_EQUAL;
		condition[0].conditionValue.type = FWP_UINT64;
		condition[0].conditionValue.uint64 = &luid.Value;

		UINT64 filterId;
		dwFwAPiRetCode = FwpmFilterAdd0(hEngine, &filter, NULL, &filterId);
		if (dwFwAPiRetCode != ERROR_SUCCESS)
		{
			Logger::instance().out(L"Utils::addPermitFilterIPv6ForTAP(), FwpmFilterAdd0 failed");
		}
	}
}

std::string readAllFromPipe(HANDLE hPipe)
{
	const int BUFFER_SIZE = 1024;
	std::string csoutput;
	while (true)
	{
		char buf[BUFFER_SIZE + 1];
		DWORD readword;
		if (!::ReadFile(hPipe, buf, BUFFER_SIZE, &readword, 0))
		{
			break;
		}
		if (readword == 0)
		{
			break;
		}
		buf[readword] = '\0';
		std::string cstemp = buf;
		csoutput += cstemp;
	}
	return csoutput;
}

std::wstring guidToStr(const GUID &guid)
{
	wchar_t szBuf[256];
	swprintf(szBuf, 256, L"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return szBuf;
}

std::wstring getExePath() 
{
	wchar_t buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

std::wstring getDirPathFromFullPath(std::wstring &fullPath)
{
	size_t found = fullPath.find_last_of(L"/\\");
	if (found != std::wstring::npos)
	{
		return fullPath.substr(0, found);
	}
	else
	{
		return fullPath;
	}
}

bool isValidFileName(std::wstring &filename)
{
	return filename.find_first_of(L"\\/<>|\":?* ") == std::wstring::npos;
}

bool isFileExists(const wchar_t *path)
{
	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool noSpacesInString(std::wstring &str)
{
	return str.find_first_of(L" ") == std::wstring::npos;
}

bool iequals(const std::wstring &a, const std::wstring &b)
{
	auto sz = static_cast<unsigned int>(a.size());
	if (b.size() != sz)
		return false;

	return _wcsnicmp(a.c_str(), b.c_str(), sz) == 0;
}

bool verifyWindscribeProcessPath(HANDLE hPipe)
{
#if defined(USE_SIGNATURE_CHECK)
    // NOTE: a test project is archived with issue 546 for testing this method.
    
   static DWORD pidVerified = 0;
   
   std::wostringstream output;

   DWORD pidClient = 0;
   BOOL result = ::GetNamedPipeClientProcessId(hPipe, &pidClient);
   if (result == FALSE)
   {
      output << "GetNamedPipeClientProcessId failed. Err = " << ::GetLastError();
      Logger::instance().out(output.str().c_str());
      return false;
   }

   if ((pidClient > 0) && (pidClient == pidVerified)) {
      return true;
   }

   WinUtils::Win32Handle processHandle(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pidClient));
   if (!processHandle.isValid())
   {
      output << "OpenProcess failed. Err = " << ::GetLastError();
      Logger::instance().out(output.str().c_str());
      return false;
   }

   wchar_t path[MAX_PATH];
   if (::GetModuleFileNameEx(processHandle.getHandle(), NULL, path, MAX_PATH) == 0)
   {
      output << "GetModuleFileNameEx failed. Err = " << ::GetLastError();
      Logger::instance().out(output.str().c_str());
      return false;
   }

   std::wstring windscribeExePath = getExePath() + std::wstring(L"\\WindscribeEngine.exe");

   if (!iequals(windscribeExePath, path))
   {
      output << "verifyWindscribeProcessPath invalid process path: " << std::wstring(path);
      Logger::instance().out(output.str().c_str());
      return false;
   }

   ExecutableSignature sigCheck;
   if (!sigCheck.verify(path))
   {
      output << "verifyWindscribeProcessPath signature verify failed. Err = " << sigCheck.lastError().c_str();
      Logger::instance().out(output.str().c_str());
      return false;
   }

   //output << "verifyWindscribeProcessPath signature verified for " << std::wstring(path);
   //Logger::instance().out(output.str().c_str());

   pidVerified = pidClient;
   return true;
#else
    (void)hPipe;
   return true;
#endif
}

void callNetworkAdapterMethod(const std::wstring &methodName, const std::wstring &adapterRegistryName)
{
	// Assume that COM has already been initialzed for the thread

	BSTR strNetworkResource = L"\\\\.\\root\\CIMV2";
	HRESULT hres;

	// Obtain the initial locator to WMI -------------------------
	IWbemLocator *pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		std::wstring output = L"Failed to create IWbemLocator object. Err = " + std::to_wstring(hres);
		Logger::instance().out(output.c_str());
		return;
	}

	// Connect to WMI through the IWbemLocator::ConnectServer method
	IWbemServices *pSvc = NULL;

	// Connect to the root\\CIMV2 namespace
	// and obtain pointer pSvc to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(strNetworkResource),      // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (e.g. Kerberos)
		0,                       // Context object
		&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres))
	{
		std::wstring output = L"Could not connect. Err = " + std::to_wstring(hres);
		Logger::instance().out(output.c_str());
		pLoc->Release();
		return;               
	}

	// Set security levels on the proxy -------------------------
	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities
	);

	if (FAILED(hres))
	{
		std::wstring output = L"Could not set proxy blanket. Err = " + std::to_wstring(hres);
		Logger::instance().out(output.c_str());

		pSvc->Release();
		pLoc->Release();
		return;
	}

	BSTR MethodName = SysAllocString(methodName.c_str());
	BSTR ClassName = SysAllocString(L"Win32_NetworkAdapter");

	// Get class
	IWbemClassObject* pClass = NULL;
	hres = pSvc->GetObject(ClassName, 0, NULL, &pClass, NULL);

	if (FAILED(hres))
	{
		std::wstring output = L"Failed GetObject IWbemClassObject. Err = " + std::to_wstring(hres);
		Logger::instance().out(output.c_str());

		SysFreeString(ClassName);
		SysFreeString(MethodName);
		pSvc->Release();
		pLoc->Release();
		return;
	}

	// Create instance
	IWbemClassObject* pClassInstance = NULL;
	hres = pClass->SpawnInstance(0, &pClassInstance);

	// Execute Method
	IWbemClassObject* pInParamsDefinition = NULL; // no input parameters to disable()
	IWbemClassObject* pOutParams = NULL;          // no output paramters to disable()

	std::wstring networkAdapterDeviceID = L"Win32_NetworkAdapter.DeviceID=\"" + adapterRegistryName + L"\"";
	hres = pSvc->ExecMethod((BSTR)networkAdapterDeviceID.c_str(), MethodName, 0,
		NULL, pClassInstance, &pOutParams, NULL);

	if (FAILED(hres))
	{
		std::wstring output = L"Could not execute method. Err = " + std::to_wstring(hres);
		Logger::instance().out(output.c_str());

		SysFreeString(ClassName);
		SysFreeString(MethodName);
		pClass->Release();
		if (pInParamsDefinition) pInParamsDefinition->Release();
		if (pOutParams) pOutParams->Release();
		pSvc->Release();
		pLoc->Release();
		return;
	}

	// Clean up
	SysFreeString(ClassName);
	SysFreeString(MethodName);
	pClass->Release();
	if (pInParamsDefinition) pInParamsDefinition->Release();
	if (pOutParams) pOutParams->Release();
	pLoc->Release();
	pSvc->Release();
	return;
}

GUID guidFromString(const std::wstring &str)
{
    GUID reqGUID;
    unsigned long p0;
    unsigned int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;

    swscanf_s(str.c_str(), L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        &p0, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    reqGUID.Data1 = p0;
    reqGUID.Data2 = static_cast<unsigned short>(p1);
    reqGUID.Data3 = static_cast<unsigned short>(p2);
    reqGUID.Data4[0] = static_cast<unsigned char>(p3);
    reqGUID.Data4[1] = static_cast<unsigned char>(p4);
    reqGUID.Data4[2] = static_cast<unsigned char>(p5);
    reqGUID.Data4[3] = static_cast<unsigned char>(p6);
    reqGUID.Data4[4] = static_cast<unsigned char>(p7);
    reqGUID.Data4[5] = static_cast<unsigned char>(p8);
    reqGUID.Data4[6] = static_cast<unsigned char>(p9);
    reqGUID.Data4[7] = static_cast<unsigned char>(p10);

    return reqGUID;
}
}
