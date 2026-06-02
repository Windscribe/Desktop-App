#include "wmi_utils.h"

#include <comdef.h>
#include <Wbemidl.h>
#include <spdlog/spdlog.h>
#include "utils/wsscopeguard.h"

namespace WmiUtils {

bool isWanIkev2AdapterDisabled()
{
    // Initialize COM
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        spdlog::error("isWanIkev2AdapterDisabled: CoInitializeEx failed: {}", hres);
        return false;
    }
    auto comCleanup = wsl::wsScopeGuard([]() { CoUninitialize(); });

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );
    if (FAILED(hres) && hres != RPC_E_TOO_LATE) {
        spdlog::error("isWanIkev2AdapterDisabled: CoInitializeSecurity failed: {}", hres);
        return false;
    }

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres)) {
        spdlog::error("isWanIkev2AdapterDisabled: CoCreateInstance(IWbemLocator) failed: {}", hres);
        return false;
    }
    auto locCleanup = wsl::wsScopeGuard([&pLoc]() { if (pLoc) pLoc->Release(); });

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        spdlog::error("isWanIkev2AdapterDisabled: ConnectServer failed: {}", hres);
        return false;
    }
    auto svcCleanup = wsl::wsScopeGuard([&pSvc]() { if (pSvc) pSvc->Release(); });

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        spdlog::error("isWanIkev2AdapterDisabled: CoSetProxyBlanket failed: {}", hres);
        return false;
    }

    // Both descriptions are compile-time string literals — there is no IPC-caller input in
    // this query. A single OR'd WHERE clause lets us answer the entire question in one round trip.
    IEnumWbemClassObject *pEnumerator = NULL;
    const wchar_t kQuery[] =
        L"SELECT ConfigManagerErrorCode FROM Win32_NetworkAdapter "
        L"WHERE Description=\"WAN Miniport (IKEv2)\" OR Description=\"WAN Miniport (IP)\"";
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(kQuery),
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        spdlog::error("isWanIkev2AdapterDisabled: ExecQuery failed: {}", hres);
        return false;
    }
    auto enumCleanup = wsl::wsScopeGuard([&pEnumerator]() { if (pEnumerator) pEnumerator->Release(); });

    while (pEnumerator) {
        IWbemClassObject *pclsObj = NULL;
        ULONG uReturn = 0;
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (FAILED(hr) || uReturn == 0) {
            break;
        }
        auto objCleanup = wsl::wsScopeGuard([&pclsObj]() { if (pclsObj) pclsObj->Release(); });

        VARIANT vtProp;
        VariantInit(&vtProp);
        auto varCleanup = wsl::wsScopeGuard([&vtProp]() { VariantClear(&vtProp); });

        hr = pclsObj->Get(L"ConfigManagerErrorCode", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_I4 && vtProp.lVal != 0) {
            spdlog::debug("isWanIkev2AdapterDisabled: WAN miniport reports ConfigManagerErrorCode={}", vtProp.lVal);
            return true;
        }
    }

    return false;
}

} // namespace WmiUtils
