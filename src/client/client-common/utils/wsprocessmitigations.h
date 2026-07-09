#pragma once

#include <Windows.h>

// Process-hardening for Windows executables
//
// The hardening runs from a TLS callback rather than a C++ global constructor.  A global
// constructor runs during CRT startup -- AFTER the loader has finished process initialization
// and released the loader lock.  That leaves a window in which an attacker who launched the
// process suspended and queued a remote thread can complete a malicious LoadLibrary before the
// constructor runs.  A TLS callback, like a DLL's DLL_PROCESS_ATTACH, is invoked by the loader
// during process initialization, under the loader lock and BEFORE the executable's entry point,
// closing that window.
//
// IMPORTANT: include this header in exactly ONE translation unit per executable.  The TLS
// callback pointer has external linkage (required so the linker keeps it), so including the
// header in two translation units of the same image is a duplicate-symbol link error.
//
// To additionally enable the process mitigation policies (Microsoft-signed DLLs only, prohibit
// dynamic code, image-load restrictions, extension-point disable, strict handle checks, and --
// on the Windows 11 SDK -- redirection trust), define WS_ENABLE_PROCESS_MITIGATIONS for
// the target.  Only enable it for executables verified to load NO non-Microsoft-signed DLL --
// neither as a static import nor at runtime, including shell-extension DLLs pulled in by
// ShellExecuteEx, common file dialogs, or other shell32 APIs.  Note these mitigations are not
// compatible with every executable: e.g. DisableExtensionPoints and MicrosoftSignedOnly assume
// no third-party in-process plugins, so do not enable for the Qt-based installer/uninstaller.
//
// Individual policies can be opted out per target (in addition to WS_ENABLE_PROCESS_MITIGATIONS)
// by defining any of the following; each disables the correspondingly named policy:
//   WS_DISABLE_SIGNATURE_POLICY            - MicrosoftSignedOnly
//   WS_DISABLE_DYNAMIC_CODE_POLICY         - ProhibitDynamicCode
//   WS_DISABLE_IMAGE_LOAD_POLICY           - image-load restrictions
//   WS_DISABLE_EXTENSION_POINT_POLICY      - DisableExtensionPoints
//   WS_DISABLE_STRICT_HANDLE_CHECK_POLICY  - strict handle checks
//   WS_DISABLE_REDIRECTION_TRUST_POLICY    - redirection trust
// This is for targets that need most of the hardening but must run a reduced set -- e.g. the
// helper service disables SIGNATURE and EXTENSION_POINT because it does in-process networking
// (wsnet) and ICS/WMI COM that can legitimately load third-party LSPs/NSPs and in-proc handlers.

namespace {

// Formats an unsigned 32-bit value as decimal into buf, which must hold at least 11 wchars (10
// digits + null).  Hand-rolled rather than using a CRT routine (e.g. _ultow_s) because this runs
// from a TLS callback BEFORE the CRT is initialized -- the loader invokes EXE TLS callbacks during
// process init, ahead of the CRT startup entry point.  This touches no CRT state (no heap, no
// locale), so it is safe at that point; only kernel32 (already loaded) is relied on elsewhere.
inline void wsFormatUInt(DWORD value, wchar_t *buf)
{
    wchar_t tmp[16];
    int i = 0;
    do {
        tmp[i++] = static_cast<wchar_t>(L'0' + (value % 10));
        value /= 10;
    } while (value != 0);

    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = L'\0';
}

// Applies one process mitigation policy and, on failure, emits a debug trace naming the policy and
// the error code (visible in DebugView or an attached debugger).  Failures are logged rather than
// fatal: a policy may legitimately be unsupported on older OS versions, and these mitigations are
// defense-in-depth on top of stronger controls (signing, /DEPENDENTLOADFLAG, admin-only dirs).
// Note a successful return means the policy was accepted, NOT that the process is free of
// non-Microsoft imports -- MicrosoftSignedOnly succeeds even with such imports already mapped and
// only blocks future loads.  lstrcatW and OutputDebugStringW are kernel32 routines (already mapped)
// and wsFormatUInt touches no CRT state, so all are safe to call from this loader-time callback.
inline void wsApplyMitigation(PROCESS_MITIGATION_POLICY policy, PVOID buffer, SIZE_T size, const wchar_t *name)
{
    if (::SetProcessMitigationPolicy(policy, buffer, size)) {
        return;
    }

    wchar_t errStr[16] = {};
    wsFormatUInt(::GetLastError(), errStr);

    wchar_t msg[160] = L"Windscribe: failed to enable process mitigation ";
    ::lstrcatW(msg, name);
    ::lstrcatW(msg, L" (");
    ::lstrcatW(msg, errStr);
    ::lstrcatW(msg, L")");
    ::OutputDebugStringW(msg);
}

void NTAPI wsOnProcessAttach(PVOID /*dllHandle*/, DWORD reason, PVOID /*reserved*/)
{
    if (reason != DLL_PROCESS_ATTACH) {
        return;
    }

    // Harden the DLL search path: drop the current directory and resolve system DLLs from
    // System32 only.  Safe for any executable, so it is always applied.
    ::SetDllDirectoryW(L"");
    ::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);

#ifdef WS_ENABLE_PROCESS_MITIGATIONS
#ifndef WS_DISABLE_SIGNATURE_POLICY
    // Block loading of any DLL not signed by Microsoft.  Note this blocks only FUTURE loads;
    // statically-imported DLLs are already mapped by the time this runs, so every static
    // import must itself be Microsoft-signed for the process to remain functional.
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY sigPolicy = {};
    sigPolicy.MicrosoftSignedOnly = 1;
    wsApplyMitigation(ProcessSignaturePolicy, &sigPolicy, sizeof(sigPolicy), L"MicrosoftSignedOnly");
#endif

#ifndef WS_DISABLE_DYNAMIC_CODE_POLICY
    // Block generation of dynamic executable code (no JIT, no RWX), and disallow a remote
    // process downgrading this one's policy.
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamicCodePolicy = {};
    dynamicCodePolicy.ProhibitDynamicCode = 1;
    dynamicCodePolicy.AllowRemoteDowngrade = 0;
    wsApplyMitigation(ProcessDynamicCodePolicy, &dynamicCodePolicy, sizeof(dynamicCodePolicy), L"ProhibitDynamicCode");
#endif

#ifndef WS_DISABLE_IMAGE_LOAD_POLICY
    // Prefer System32 copies of system DLLs, and refuse images loaded from remote (UNC/network)
    // locations or written by a low-integrity process.
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY imageLoadPolicy = {};
    imageLoadPolicy.PreferSystem32Images = 1;
    imageLoadPolicy.NoRemoteImages = 1;
    imageLoadPolicy.NoLowMandatoryLabelImages = 1;
    wsApplyMitigation(ProcessImageLoadPolicy, &imageLoadPolicy, sizeof(imageLoadPolicy), L"ImageLoad");
#endif

#ifndef WS_DISABLE_EXTENSION_POINT_POLICY
    // Block the legacy injection/extension points that MicrosoftSignedOnly does not by itself
    // cover: AppInit_DLLs, global SetWindowsHookEx hooks, legacy IMEs, and Winsock LSPs.
    PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY extPointPolicy = {};
    extPointPolicy.DisableExtensionPoints = 1;
    wsApplyMitigation(ProcessExtensionPointDisablePolicy, &extPointPolicy, sizeof(extPointPolicy), L"ExtensionPointDisable");
#endif

#ifndef WS_DISABLE_STRICT_HANDLE_CHECK_POLICY
    // Turn use of an invalid/closed handle into an immediate exception rather than a silent
    // failure, hardening against handle confusion and use-after-close.
    PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY handlePolicy = {};
    handlePolicy.RaiseExceptionOnInvalidHandleReference = 1;
    handlePolicy.HandleExceptionsPermanentlyEnabled = 1;
    wsApplyMitigation(ProcessStrictHandleCheckPolicy, &handlePolicy, sizeof(handlePolicy), L"StrictHandleCheck");
#endif

#if defined(NTDDI_WIN10_CO) && !defined(WS_DISABLE_REDIRECTION_TRUST_POLICY)
    // Redirection Guard: refuse to follow filesystem/registry junctions and symlinks created by
    // lower-privileged users, defending this (elevated) process's file operations against
    // symlink/junction redirection attacks.  Requires the Windows 11 SDK to compile; on OS
    // versions that predate the policy the call fails harmlessly and is logged as a failure.
    PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY redirPolicy = {};
    redirPolicy.EnforceRedirectionTrust = 1;
    wsApplyMitigation(ProcessRedirectionTrustPolicy, &redirPolicy, sizeof(redirPolicy), L"RedirectionTrust");
#endif

    // Trace that mitigations were applied, naming the executable (visible in DebugView or an
    // attached debugger).  GetModuleFileNameW, lstrcatW, and OutputDebugStringW are kernel32
    // routines already mapped, so they are safe to call from this loader-time callback.
    wchar_t exePath[MAX_PATH] = {};
    ::GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    wchar_t msg[MAX_PATH + 64] = L"Windscribe: process mitigations enabled for ";
    ::lstrcatW(msg, exePath);
    ::OutputDebugStringW(msg);
#endif
}

} // namespace

// Emit the TLS directory (/INCLUDE:_tls_used) and register our callback in the TLS callback
// array (.CRT$XLB) so the loader invokes it before the executable's entry point.  The second
// /INCLUDE forces the linker to keep the callback pointer.  '_tls_used' and the callback
// symbol are undecorated on x64/arm64 (the only Windows targets); this is not valid for x86.
#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:ws_tls_callback")
#pragma const_seg(".CRT$XLB")
extern "C" const PIMAGE_TLS_CALLBACK ws_tls_callback = wsOnProcessAttach;
#pragma const_seg()
