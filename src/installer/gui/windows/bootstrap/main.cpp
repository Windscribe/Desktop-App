#include <Windows.h>

#include <fileapi.h>
#include <filesystem>
#include <optional>
#include <string>
#include <tchar.h>
#include <versionhelpers.h>

#include "../utils/archive.h"
#include "../utils/secure_dir.h"
#include "global_consts.h"
#include "win32handle.h"
#include "wsprocessmitigations.h"
#include "wsscopeguard.h"

static std::optional<bool> isElevated()
{
    wsl::Win32Handle token;
    BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.data());
    if (result == FALSE) {
        return std::nullopt;
    }

    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    result = GetTokenInformation(token.getHandle(), TokenElevation, &Elevation, sizeof(Elevation), &cbSize);
    if (result == FALSE) {
        return std::nullopt;
    }

    return Elevation.TokenIsElevated;
}

static int showMessageBox(HWND hOwner, LPCTSTR szTitle, UINT nStyle, LPCTSTR szFormat, ...)
{
    va_list arg_list;
    va_start(arg_list, szFormat);

    TCHAR Buf[1024];
    _vsntprintf_s(Buf, 1024, _TRUNCATE, szFormat, arg_list);
    va_end(arg_list);

    int nResult = ::MessageBox(hOwner, Buf, szTitle, nStyle);

    return nResult;
}

static void debugMessage(LPCTSTR szFormat, ...)
{
    va_list arg_list;
    va_start(arg_list, szFormat);

    TCHAR Buf[1024];
    _vsntprintf_s(Buf, 1024, _TRUNCATE, szFormat, arg_list);
    va_end(arg_list);

    // Send the debug string to the debugger.
    ::OutputDebugString(Buf);
}

// Launch exePath with the given command-line parameters and wait for it to exit, returning
// the child's exit code via exitCode.  Uses CreateProcess rather than ShellExecuteEx so that
// no shell32 code -- and therefore no third-party shell-extension DLL -- is loaded into this
// process.  That is what keeps the bootstrapper compatible with the Microsoft-signed-only
// load policy applied at startup (see wsprocessmitigations.h).  The child inherits this process's
// (elevated) token, so no shell "runas" verb is needed to run it as administrator.
static DWORD launchAndWait(const std::wstring &exePath, const wchar_t *params, DWORD *exitCode)
{
    std::wstring cmdLine = L"\"" + exePath + L"\"";
    if (params != nullptr && params[0] != L'\0') {
        cmdLine += L' ';
        cmdLine += params;
    }

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    // CreateProcessW may modify the command-line buffer in place, so it must be writable.
    BOOL result = ::CreateProcessW(exePath.c_str(), cmdLine.data(), nullptr, nullptr, FALSE,
                                   0, nullptr, nullptr, &si, &pi);
    if (!result) {
        const DWORD err = ::GetLastError();
        debugMessage(_T("Windscribe Installer - CreateProcess failed: %lu"), err);
        return err;
    }

    wsl::Win32Handle threadHandle(pi.hThread);
    wsl::Win32Handle processHandle(pi.hProcess);

    processHandle.wait(INFINITE);
    if (exitCode != nullptr) {
        if (!::GetExitCodeProcess(processHandle.getHandle(), exitCode)) {
            *exitCode = 0;
            debugMessage(_T("Windscribe Installer - GetExitCodeProcess failed: %lu"), ::GetLastError());
        }
    }

    return NO_ERROR;
}

static void deleteFolder(const std::filesystem::path &folder)
{
    if (folder.empty()) {
        return;
    }

    // Use std::filesystem rather than SHFileOperation so that no shell32 code is loaded into
    // this process (see launchAndWait).
    std::error_code ec;
    std::filesystem::remove_all(folder, ec);
    if (ec) {
        debugMessage(_T("Windscribe Installer - deleteFolder remove_all failed: %d"), ec.value());
    }
}

static bool isOSCompatible()
{
    NTSTATUS (WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);
    *(FARPROC*)&RtlGetVersion = ::GetProcAddress(::GetModuleHandleA("ntdll"), "RtlGetVersion");
    if (!RtlGetVersion) {
        // Shouldn't ever get here... but if we do we at least know they are on >= Windows 10, and
        // will assume the OS build is compatible.
        debugMessage(L"Windscribe installer failed to load RtlGetVersion");
        return true;
    }

    RTL_OSVERSIONINFOEXW rtlOsVer;
    ::ZeroMemory(&rtlOsVer, sizeof(RTL_OSVERSIONINFOEXW));
    rtlOsVer.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    RtlGetVersion(&rtlOsVer);
    // Windows 11 build numbers continue from the W10 build numbers, so no need to check the OS version here.
    return (rtlOsVer.dwBuildNumber >= kMinWindowsBuildNumber);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpszCmdParam, int nCmdShow)
{
    if (!IsWindows10OrGreater()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The Windscribe app cannot be installed on this version of Windows.  It requires Windows 10 or newer."));
        return -1;
    }

    // The executable requests requireAdministrator, so the OS elevates this process at launch.  Verify elevation defensively.
    const auto isAdmin = isElevated();
    if (!isAdmin.has_value()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Couldn't determine if user has administrative rights."));
        return -1;
    }
    if (!isAdmin.value()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The Windscribe installer requires administrator privileges to run."));
        return -1;
    }

    if (!isOSCompatible()) {
        auto response = showMessageBox(NULL, _T("Windscribe Installer"), MB_YESNO | MB_ICONWARNING,
                                       _T("The Windscribe app may not function correctly on this version of Windows.  It requires Windows 10"
                                          " build %lu or newer for full functionality.\n\nDo you want to proceed with the install?"), kMinWindowsBuildNumber);
        if (response != IDYES) {
            return -1;
        }
    }

    std::filesystem::path installPath;
    auto exitGuard = wsl::wsScopeGuard([&] {
        deleteFolder(installPath);
    });

    // Place the installer under C:\Windows\Temp in an Administrators+SYSTEM-only directory.
    auto randomSuffix = wsl::generateRandomSuffix();
    if (!randomSuffix) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to generate a secure random suffix for the installer directory."));
        return -1;
    }

    {
        // Use GetWindowsDirectory rather than SHGetKnownFolderPath so that no shell32 code is
        // loaded into this process (see launchAndWait).
        wchar_t windowsDir[MAX_PATH];
        const UINT len = ::GetWindowsDirectoryW(windowsDir, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't locate Windows directory (%lu)"), ::GetLastError());
            return -1;
        }

        installPath = windowsDir;
        installPath.append(L"Temp");
        installPath.append(L"WindscribeInstaller" + randomSuffix.value());

        // Atomically create the install directory with a DACL granting access to Administrators and
        // SYSTEM only, so there is no window in which a lower-privileged process can place files in it.
        // Fails if the directory already exists (with 128 bits of randomness, a collision means the
        // path leaked or RNG is broken).
        auto logFn = [](const std::wstring &msg) {
            ::OutputDebugStringW(msg.c_str());
        };
        DWORD err = wsl::createAdminOnlyDirectory(installPath, logFn);
        if (err == ERROR_ALREADY_EXISTS) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("The installer directory '%s' already exists.  This is unexpected and may indicate")
                           _T(" a tampering attempt.  Please reboot and try again, and contact Windscribe support")
                           _T(" if the problem persists."), installPath.c_str());
            return -1;
        }
        if (err != ERROR_SUCCESS) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Failed to create the installer directory '%s' (%lu)."), installPath.c_str(), err);
            return -1;
        }
    }

    debugMessage(L"Windscribe bootstrapper installing to %s", installPath.c_str());

    // Extract archive
    wsl::Archive archive;
    archive.setLogFunction([](const std::wstring &str) {
        debugMessage(str.c_str());
    });

    if (!archive.extractDecompressor(installPath)) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to extract the Windscribe decompressor."));
        return -1;
    }

    if (!archive.extract(L"Installer", L"windscribeinstaller.7z", installPath, installPath)) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to extract the Windscribe installer."));
        return -1;
    }

    std::filesystem::path app(installPath);
    app.append(WINDSCRIBE_INSTALLER_NAME);

    DWORD exitCode = 0;
    auto result = launchAndWait(app.wstring(), lpszCmdParam, &exitCode);
    if (result != NO_ERROR) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Windows was unable to launch the installer (%lu)"), result);
        return -1;
    }

    if (exitCode != 0) {
        debugMessage(_T("Windscribe installer exited with an error: %lu"), exitCode);
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The installer reported an unexpected exit code, indicating it may have crashed during exit.")
                       _T(" Please report this failure to Windscribe support."));
    }

    return exitCode;
}
