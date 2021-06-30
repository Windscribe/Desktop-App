#include "exports.h"
#include "components.h"
#include "guids.h"

#include <strsafe.h>

const std::wstring installPath = L"C:\\Program Files (x86)\\Windscribe";
const std::wstring comDllName       = L"ws_com.dll";
const std::wstring comProxyStubName = L"ws_proxy_stub.dll";
const std::wstring comServerName    = L"ws_com_server.exe";

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv)
{
	std::cout << "DllGetClassObject() called" << std::endl;

	if (clsid != CLSID_AUTH_HELPER)
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	CAuthHelperFactory *pFactory = new CAuthHelperFactory;
	if (pFactory == NULL)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pFactory->QueryInterface(iid, ppv);
	pFactory->Release();
	return hr;
}

STDAPI DllCanUnloadNow()
{
	if (g_cComponents == 0 && g_cServerLocks == 0)
	{
		return S_OK;
	}
	return S_FALSE;
}

STDAPI RegisterServerWithTargetPaths(const wchar_t * comPath, const wchar_t * proxyPath, const wchar_t * serverPath)
{
	// startConsole();
	// CLSID reg entries are added to registry by default by running regsvr32 against this dll

	// get CLSID authhelper
	WCHAR *lpwszClsid;
	WCHAR szClsid[MAX_PATH] = L"";
	StringFromCLSID(CLSID_AUTH_HELPER, &lpwszClsid);
	wsprintf(szClsid, L"%s", lpwszClsid);

	// get iid authhelper
	WCHAR *lpwszIidComponentInterface;
	StringFromCLSID(IID_AUTH_HELPER, &lpwszIidComponentInterface);
	WCHAR szIid[MAX_PATH] = L"";
	wsprintf(szIid, L"%s", lpwszIidComponentInterface);

	// CLSID subkey
	WCHAR szClsidSubKey[MAX_PATH] = L"";
	wsprintf(szClsidSubKey, L"%s\\%s", L"Clsid", szClsid);

	// IID subkey
	WCHAR szIidSubKey[MAX_PATH] = L"";
	wsprintf(szIidSubKey, L"%s\\%s", L"Interface", szIid);

	WCHAR szComponentName[MAX_PATH] = L"";
	WCHAR szInproc[MAX_PATH] = L"";
	WCHAR szLocalServer[MAX_PATH] = L"";
	WCHAR szElevation[MAX_PATH] = L"";
	WCHAR szDescriptionVal[256] = L"";
	WCHAR szLocalizedString[MAX_PATH] = L"";
	WCHAR szIconReferenceString[MAX_PATH] = L"";

	wsprintf(szInproc, L"%s\\%s\\%s", L"clsid", szClsid, L"InprocServer32");
	wsprintf(szLocalServer, L"%s\\%s\\%s", L"clsid", szClsid, L"LocalServer32");
	wsprintf(szElevation, L"%s\\%s\\%s", L"clsid", szClsid, L"Elevation");

	WCHAR szComDllPath[MAX_PATH] = L"";
	WCHAR szStubProxyPath[MAX_PATH] = L"";
	WCHAR szComServerPath[MAX_PATH] = L"";
	wsprintf(szComDllPath, L"%s\\%s", comPath, comDllName.c_str());
	wsprintf(szStubProxyPath, L"%s\\%s", proxyPath, comProxyStubName.c_str());
	wsprintf(szComServerPath, L"%s\\%s", serverPath, comServerName.c_str());

	wsprintf(szLocalizedString, L"@%s,-101", szComDllPath);
	wsprintf(szIconReferenceString, L"@%s,-102", szComDllPath);

	// CLSID\{CLSID} 
	wsprintf(szComponentName, L"%s", L"Windscribe CAuthHelper");
	wsprintf(szDescriptionVal, L"%s\\%s", L"clsid", szClsid);
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, NULL, REG_SZ, szComponentName, wcslen(szComponentName)) == FALSE) return S_FALSE;

	// CLSID\{CLSID} LocalizedString -> com.dll
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, L"LocalizedString", REG_SZ, szLocalizedString, wcslen(szLocalizedString)) == FALSE) return S_FALSE;

	// CLSID\{CLSID}\InprocServer32 (default) -> proxy_stub.dll
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szInproc, NULL, REG_SZ, szStubProxyPath, wcslen(szStubProxyPath)) == FALSE) return S_FALSE;

	// CLSID\{CLSID}\LocalServer32 (default) -> com_server.exe
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szLocalServer, NULL, REG_SZ, szComServerPath, wcslen(szComServerPath)) == FALSE) return S_FALSE;

	// CLSID\{CLSID}\Elevation Enabled -> 1
	if (HelperWriteKeyDword(HKEY_CLASSES_ROOT, szElevation, L"Enabled", 1) == FALSE) return S_FALSE;

	// CLSID\{CLSID}\Elevation IconReference -> com.dll
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szElevation, L"IconReference", REG_SZ, szIconReferenceString, wcslen(szIconReferenceString)) == FALSE) return S_FALSE;

	// Also register proxy-stub registry entries so we don't have to run two dlls:
	// Interface\{IID} (default) -> IAuthHelper
	WCHAR szComponentInterfaceName[MAX_PATH] = L"";
	wsprintf(szComponentInterfaceName, L"%s", L"IAuthHelper");
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szIidSubKey, NULL, REG_SZ, szComponentInterfaceName, wcslen(szComponentInterfaceName)) == FALSE) return S_FALSE;

	// Interface\{IID}\NumMethods (default) -> "3"
	WCHAR szInterfaceIidNumMethods[MAX_PATH] = L"";
	wsprintf(szInterfaceIidNumMethods, L"%s\\%s\\%s", L"Interface", szIid, L"NumMethods");
	WCHAR szNumMethodsValue[MAX_PATH] = L"3";
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szInterfaceIidNumMethods, NULL, REG_SZ, szNumMethodsValue, wcslen(szNumMethodsValue)) == FALSE) return S_FALSE;

	// There is something that doesn't work about the following commented code
	// -- ideally this should work and there will be no reason to use regsvr32 on the proxy_stub
	// -- strangely: manually reinputting the same CLSID will allow it to work :S
	// Interface\{IID}\ProxyStubClsid32  (default) -> CLSID
	// WCHAR szProxyStubClsidKey[MAX_PATH] = L"";
	// wsprintf(szProxyStubClsidKey, L"%s\\%s\\%s", L"Interface", szIid, L"ProxyStubClsid32");
	// if (HelperWriteKey(HKEY_CLASSES_ROOT, szProxyStubClsidKey, NULL, REG_SZ, szClsid, wcslen(szClsid)) == FALSE) return S_FALSE;

	// S_FALSE doesn't seem to make failure occur in regsvr32 -- might be better exit code for fail cases
	return S_OK;
}

STDAPI DllRegisterServer()
{
#ifdef NDEBUG
	RegisterServerWithTargetPaths(installPath.c_str(), installPath.c_str(), installPath.c_str());
#else
	const std::wstring comDllPath       = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_com\\Debug";
	const std::wstring comProxyStubPath = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_proxy_stub\\Debug";
	const std::wstring comServerPath    = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_com_server\\Debug";
	RegisterServerWithTargetPaths(comDllPath.c_str(), comProxyStubPath.c_str(), comServerPath.c_str());
#endif
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	// get CLSID authhelper
	WCHAR *lpwszClsid;
	WCHAR szClsid[MAX_PATH] = L"";
	StringFromCLSID(CLSID_AUTH_HELPER, &lpwszClsid);
	wsprintf(szClsid, L"%s", lpwszClsid);

	WCHAR szClsidSubKey[MAX_PATH] = L"";
	wsprintf(szClsidSubKey, L"%s\\%s", L"CLSID", szClsid);

	// non-recursive delete will fail due to inner subkeys -- use manual recursive delete
	if (HelperDeleteKeyRecursive(HKEY_CLASSES_ROOT, szClsidSubKey) == FALSE)
	{
		return S_FALSE;
	}

	// get iid authhelper
	WCHAR *lpwszIidComponentInterface;
	StringFromCLSID(IID_AUTH_HELPER, &lpwszIidComponentInterface);
	WCHAR szIid[MAX_PATH] = L"";
	wsprintf(szIid, L"%s", lpwszIidComponentInterface);

	WCHAR szIidSubKey[MAX_PATH] = L"";
	wsprintf(szIidSubKey, L"%s\\%s", L"Interface", szIid);

	// non-recursive delete will fail due to inner subkeys -- use manual recursive delete
	if (HelperDeleteKeyRecursive(HKEY_CLASSES_ROOT, szIidSubKey) == FALSE)
	{
		return S_FALSE;
	}

	return S_OK;
}

STDAPI StartFactories()
{
	BOOL result = CAuthHelperFactory::StartFactories();
	if (result == TRUE)
		return S_OK;
	return S_FALSE;
}

STDAPI StopFactories()
{
	CAuthHelperFactory::StopFactories();
	return S_OK;
}

BOOL HelperDeleteKeyRecursive(HKEY roothk, wchar_t *lpSubKey)
{
	WCHAR * lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having to recurse.
	lResult = RegDeleteKey(roothk, lpSubKey);

	if (lResult == ERROR_SUCCESS)
	{
		wprintf(TEXT("removed %s\n"), lpSubKey);
		return TRUE;
	}

	lResult = RegOpenKeyEx(roothk, lpSubKey, 0, KEY_READ, &hKey);

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
	// cppcheck-suppress lstrlenCalled
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

			if (!HelperDeleteKeyRecursive(roothk, lpSubKey)) {
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
	lResult = RegDeleteKey(roothk, lpSubKey);
	if (lResult == ERROR_SUCCESS)
		return TRUE;

	return FALSE;
}


BOOL HelperWriteKey(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD dwType, const wchar_t *lpvData, DWORD dwDataSize)
{
	HKEY hk;
	if (ERROR_SUCCESS != RegCreateKey(roothk, lpSubKey, &hk)) return FALSE;
	if (ERROR_SUCCESS != RegSetValueEx(hk, val_name, 0, dwType, (CONST BYTE *) lpvData, sizeof(lpvData)*dwDataSize))  return FALSE;
	if (ERROR_SUCCESS != RegCloseKey(hk)) return FALSE;
	return TRUE;
}

BOOL HelperWriteKeyDword(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD value)
{
	HKEY hk;
	if (ERROR_SUCCESS != RegCreateKey(roothk, lpSubKey, &hk)) return FALSE;
	if (ERROR_SUCCESS != RegSetValueEx(hk, val_name, 0, REG_DWORD, (CONST BYTE *) &value, sizeof(value)))  return FALSE;
	if (ERROR_SUCCESS != RegCloseKey(hk)) return FALSE;
	return TRUE;
}

void startConsole()
{
	// handy for debugging: to get regsvr32.exe to show output
	AllocConsole();
	FILE *stream;
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
}

#ifdef __cplusplus
}
#endif

