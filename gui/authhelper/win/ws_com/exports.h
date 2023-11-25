#pragma once

#include <objbase.h>
#include <string>

#ifdef __cplusplus    // If used by C++ code,
extern "C" {          // we need to export the C interface
#endif

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv);
STDAPI DllCanUnloadNow();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();

STDAPI RegisterServerWithTargetPaths(const std::wstring &comPath, const std::wstring &proxyPath, const std::wstring &serverPath);

STDAPI StartFactories();
STDAPI StopFactories();

BOOL HelperDeleteKeyRecursive(HKEY roothk, wchar_t *lpSubKey);
BOOL HelperWriteKey(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD dwType, const wchar_t *lpvData, DWORD dwDataSize);
BOOL HelperWriteKeyDword(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD value);

void startConsole();

#ifdef __cplusplus
}
#endif
