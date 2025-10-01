#include "all_headers.h"
#include "wmi_utils.h"

#include <comdef.h>
#include <Wbemidl.h>
#include <spdlog/spdlog.h>
#include "utils/wsscopeguard.h"

#pragma comment(lib, "wbemuuid.lib")

namespace WmiUtils {

std::wstring getNetworkAdapterConfigManagerErrorCode(const std::wstring& adapterName)
{
    spdlog::debug(L"WmiUtils::getNetworkAdapterConfigManagerErrorCode, adapter={}", adapterName);
    
    std::wstring result;
    
    // Initialize COM
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        spdlog::error("Failed to initialize COM library. Error code = {}", hres);
        return L"";
    }
    auto comCleanup = wsl::wsScopeGuard([]() { CoUninitialize(); });

    // Set general COM security levels
    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        spdlog::error("Failed to initialize security. Error code = {}", hres);
        return L"";
    }

    // Obtain the initial locator to WMI
    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres)) {
        spdlog::error("Failed to create IWbemLocator object. Error code = {}", hres);
        return L"";
    }
    auto locCleanup = wsl::wsScopeGuard([&pLoc]() { if (pLoc) pLoc->Release(); });

    // Connect to WMI
    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        spdlog::error("Could not connect to WMI. Error code = {}", hres);
        return L"";
    }
    auto svcCleanup = wsl::wsScopeGuard([&pSvc]() { if (pSvc) pSvc->Release(); });

    // Set security levels on the proxy
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        spdlog::error("Could not set proxy blanket. Error code = {}", hres);
        return L"";
    }

    // Execute WMI query
    IEnumWbemClassObject* pEnumerator = NULL;
    std::wstring query = L"SELECT ConfigManagerErrorCode FROM Win32_NetworkAdapter WHERE Description=\"" + adapterName + L"\"";
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()),
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        spdlog::error("WMI query failed. Error code = {}", hres);
        return L"";
    }
    auto enumCleanup = wsl::wsScopeGuard([&pEnumerator]() { if (pEnumerator) pEnumerator->Release(); });

    // Get the data from the query
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (uReturn > 0) {
        auto objCleanup = wsl::wsScopeGuard([&pclsObj]() { if (pclsObj) pclsObj->Release(); });
        
        VARIANT vtProp;
        VariantInit(&vtProp);
        auto varCleanup = wsl::wsScopeGuard([&vtProp]() { VariantClear(&vtProp); });

        hr = pclsObj->Get(L"ConfigManagerErrorCode", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
            result = L"ConfigManagerErrorCode\r\n" + std::to_wstring(vtProp.lVal) + L"\r\n";
        }
    }

    return result;
}

} // namespace WmiUtils