// dllmain.cpp : Defines the entry point for the DLL application.

#include <Windows.h>
#include <tchar.h>

static void debugMessage(LPCTSTR szFormat, ...)
{
    va_list arg_list;
    va_start(arg_list, szFormat);

    TCHAR Buf[1024];
    _vsntprintf_s(Buf, 1024, _TRUNCATE, szFormat, arg_list);
    va_end(arg_list);

    ::OutputDebugString(Buf);
}

static void OnlyLoadMSSignedBinaries()
{
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sigPolicy = {};
    sigPolicy.MicrosoftSignedOnly = 1;
    BOOL result = ::SetProcessMitigationPolicy(ProcessSignaturePolicy, &sigPolicy, sizeof(sigPolicy));
    if (!result) {
        debugMessage(L"**wssecure** SetProcessMitigationPolicy(MicrosoftSignedOnly) failed (%lu)", ::GetLastError());
    }

    // This mitigation by itself does not block DLL injection.
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {};
    dynamicCodePolicy.AllowRemoteDowngrade = 0;
    dynamicCodePolicy.ProhibitDynamicCode = 1;
    result = ::SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy));
    if (!result) {
        debugMessage(L"**wssecure** SetProcessMitigationPolicy(Dynamic Code) failed (%lu)", ::GetLastError());
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        OnlyLoadMSSignedBinaries();
        ::DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
