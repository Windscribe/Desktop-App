#include "exports.h"
#include "components.h"
#include "guids.h"

#include <strsafe.h>

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

STDAPI DllRegisterServer()
{
	WCHAR *lpwszClsid;
	WCHAR szBuff[MAX_PATH] = L"";
	WCHAR szClsid[MAX_PATH] = L"";
	WCHAR szInproc[MAX_PATH] = L"";
	WCHAR szLocalServer[MAX_PATH] = L"";
	WCHAR szElevation[MAX_PATH] = L"";
	WCHAR szDescriptionVal[256] = L"";
	WCHAR szLocalizedString[MAX_PATH] = L"";
	WCHAR szIconReferenceString[MAX_PATH] = L"";

	StringFromCLSID(CLSID_AUTH_HELPER, &lpwszClsid);
	wsprintf(szClsid, L"%s", lpwszClsid);
	wsprintf(szInproc, L"%s\\%s\\%s", L"clsid", szClsid, L"InprocServer32");
	wsprintf(szLocalServer, L"%s\\%s\\%s", L"clsid", szClsid, L"LocalServer32");
	wsprintf(szElevation, L"%s\\%s\\%s", L"clsid", szClsid, L"Elevation");

	// TODO: add different paths for release version
	WCHAR szComDllPath[MAX_PATH] = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_com\\Debug\\ws_com.dll";
	WCHAR szStubProxyPath[MAX_PATH] = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_proxy_stub\\Debug\\ws_proxy_stub.dll";
	WCHAR szComServerPath[MAX_PATH] = L"C:\\work\\client-desktop\\gui\\authhelper\\ws_com_server\\Debug\\ws_com_server.exe";
	wsprintf(szLocalizedString, L"@%s,-101", szComDllPath);
	wsprintf(szIconReferenceString, L"@%s,-102", szComDllPath);

	// write the default value (name)
	wsprintf(szBuff, L"%s", L"ws_com");
	wsprintf(szDescriptionVal, L"%s\\%s", L"clsid", szClsid);
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, NULL, REG_SZ, szBuff, wcslen(szBuff)) == FALSE)
	{
		return S_FALSE;
	}

	// write LocalizedString
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szDescriptionVal, L"LocalizedString", REG_SZ, szLocalizedString, wcslen(szLocalizedString)) == FALSE)
	{
		return S_FALSE;
	}

	// write the "InprocServer32" key data
	// GetModuleFileName(g_hModule, szBuff, sizeof(szBuff));
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szInproc, NULL, REG_SZ, szStubProxyPath, wcslen(szStubProxyPath)) == FALSE)
	{
		return S_FALSE;
	}

	// write LocalServer32 key data
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szLocalServer, NULL, REG_SZ, szComServerPath, wcslen(szComServerPath)) == FALSE)
	{
		return S_FALSE;
	}

	// write Elevation "Enabled" key
	DWORD value = 1;
	if (HelperWriteKeyDword(HKEY_CLASSES_ROOT, szElevation, L"Enabled", value) == FALSE)
	{
		return S_FALSE;
	}

	// write IconReference key
	if (HelperWriteKey(HKEY_CLASSES_ROOT, szElevation, L"IconReference", REG_SZ, szIconReferenceString, wcslen(szIconReferenceString)) == FALSE)
	{
		return S_FALSE;
	}

	// S_FALSE doesn't seem to make failure occur in regsvr32 -- might be better exit code for fail cases
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	// TODO: undo all that is done in Register

	WCHAR *lpwszClsid;
	WCHAR szClsid[MAX_PATH] = L"";

	StringFromCLSID(CLSID_AUTH_HELPER, &lpwszClsid);
	wsprintf(szClsid, L"%s\\%s", L"clsid", lpwszClsid);

	// non-recursive delete will fail due to inner subkeys -- use manual recursive delete
	if (HelperDeleteKeyRecursive(HKEY_CLASSES_ROOT, szClsid) == FALSE)
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

