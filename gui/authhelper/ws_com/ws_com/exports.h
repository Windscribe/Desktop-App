#pragma once

#include <objbase.h>

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

STDAPI DllGetClassObject(const CLSID &clsid, const IID &iid, void **ppv);
STDAPI DllCanUnloadNow();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();

STDAPI RegisterServerWithTargetPaths(const wchar_t * comPath, const wchar_t * proxyPath, const wchar_t * serverPath);

STDAPI StartFactories();
STDAPI StopFactories();

BOOL HelperDeleteKeyRecursive(HKEY roothk, wchar_t *lpSubKey);
BOOL HelperWriteKey(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD dwType, const wchar_t *lpvData, DWORD dwDataSize);
BOOL HelperWriteKeyDword(HKEY roothk, const wchar_t *lpSubKey, LPCWSTR val_name, DWORD value);

void startConsole();

#ifdef __cplusplus
}

#endif