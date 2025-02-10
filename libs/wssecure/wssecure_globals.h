#pragma once

// Defines global scope structs to be included in any app that is unable to utilize the wssecure library.
// E.g. the Windows bootstrapper and uninstaller which are designed to be stand-alone executables.

// Disabled for now since the mitigation is blocking 'legitimate' DLL injection (e.g. FileZilla Pro's shell extension DLL).
/*
struct ProcessMitigation
{
    ProcessMitigation()
    {
        ::OutputDebugString(L"** Setting process mitigation policies **");
        PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sp = {};
        sp.MicrosoftSignedOnly = 1;
        BOOL result = ::SetProcessMitigationPolicy(ProcessSignaturePolicy, &sp, sizeof(sp));
        if (!result) {
            ::OutputDebugString(L"SetProcessMitigationPolicy(ProcessSignaturePolicy) failed");
        }

        // This mitigation by itself does not block DLL injection.
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY PMDCP = {};
        PMDCP.AllowRemoteDowngrade = 0;
        PMDCP.ProhibitDynamicCode = 1;
        result = ::SetProcessMitigationPolicy(ProcessDynamicCodePolicy, &PMDCP, sizeof(PMDCP));
        if (!result) {
            ::OutputDebugString(L"SetProcessMitigationPolicy(ProcessDynamicCodePolicy) failed");
        }

        PROCESS_MITIGATION_IMAGE_LOAD_POLICY ilp = {};
        ilp.PreferSystem32Images = 1;
        result = ::SetProcessMitigationPolicy(ProcessImageLoadPolicy, &ilp, sizeof(ilp));
        if (!result) {
            ::OutputDebugString(L"SetProcessMitigationPolicy(ProcessImageLoadPolicy) failed");
        }
    }
} processMitigation;
*/


// Set the DLL load directory to the system directory before entering WinMain().
struct LoadSystemDLLsFromSystem32
{
    LoadSystemDLLsFromSystem32()
    {
        // Remove the current directory from the search path for dynamically loaded
        // DLLs as a precaution.  This call has no effect for delay load DLLs.
        ::SetDllDirectory(L"");
        ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
} loadSystemDLLs;
