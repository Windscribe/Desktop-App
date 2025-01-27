#include "wssecure.h"

#include <Windows.h>

// This function export must be called from the appliation to ensure the loader actually loads the DLL.
// The loader (linker?) will skip loading the DLL if no references to its function exports are made.
WSSECURE_EXPORT bool verifyProcessMitigations(std::wstring &errorMsg)
{
    const auto procHandle = ::GetCurrentProcess();

    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sigPolicy = {};
    auto result = ::GetProcessMitigationPolicy(procHandle, ProcessSignaturePolicy, &sigPolicy, sizeof(sigPolicy));
    if (!result) {
        errorMsg = L"GetProcessMitigationPolicy(MicrosoftSignedOnly) failed: " + std::to_wstring(::GetLastError());
        return false;
    }

    if (!sigPolicy.MicrosoftSignedOnly) {
        errorMsg = L"MicrosoftSignedOnly mitigation is not enabled for the process";
        return false;
    }

    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {};
    result = ::GetProcessMitigationPolicy(procHandle, ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy));
    if (!result) {
        errorMsg = L"GetProcessMitigationPolicy(ProcessDynamicCodePolicy) failed: " + std::to_wstring(::GetLastError());
        return false;
    }

    if (dynamicCodePolicy.ProhibitDynamicCode != 1) {
        errorMsg = L"ProhibitDynamicCode mitigation is not enabled for the process";
        return false;
    }

    if (dynamicCodePolicy.AllowRemoteDowngrade != 0) {
        errorMsg = L"AllowRemoteDowngrade mitigation is not enabled for the process";
        return false;
    }

    return true;
}
