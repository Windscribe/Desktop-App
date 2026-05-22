#include <Windows.h>

#include <fileapi.h>
#include <filesystem>
#include <optional>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <tchar.h>
#include <versionhelpers.h>

#include "../utils/archive.h"
#include "../utils/secure_dir.h"
#include "global_consts.h"
#include "wsscopeguard.h"
#include "win32handle.h"

#include "../../../../libs/wssecure/wssecure_globals.h"

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

static DWORD executeProgram(const wchar_t *cmd, const wchar_t *params, bool wait, bool asAdmin, LPDWORD exitCode)
{
    SHELLEXECUTEINFO ei;
    memset(&ei, 0, sizeof(SHELLEXECUTEINFO));

    ei.cbSize = sizeof(SHELLEXECUTEINFO);
    ei.fMask = SEE_MASK_NOCLOSEPROCESS;
    ei.lpFile = cmd;
    ei.lpParameters = params;
    ei.nShow = SW_RESTORE;
    if (asAdmin) {
        ei.lpVerb = L"runas";
    }

    BOOL result = ShellExecuteEx(&ei);

    if (!result) {
        debugMessage(_T("Windscribe Installer - ShellExecuteEx failed: %lu"), GetLastError());
        return GetLastError();
    }

    wsl::Win32Handle processHandle(ei.hProcess);

    if (wait && processHandle.isValid()) {
        auto waitResult = processHandle.wait(INFINITE);
        if (waitResult == WAIT_OBJECT_0 && exitCode != nullptr) {
            result = ::GetExitCodeProcess(processHandle.getHandle(), exitCode);
            if (!result) {
                exitCode = 0;
                debugMessage(_T("Windscribe Installer - GetExitCodeProcess failed: %lu"), GetLastError());
            }
        }
    }

    return NO_ERROR;
}

static void deleteFolder(const std::filesystem::path &folder)
{
    if (folder.empty()) {
        return;
    }

    // SHFileOperation requires file names to be terminated with two \0s
    std::wstring dir = folder.c_str();
    dir += L'\0';

    SHFILEOPSTRUCT fos;
    memset(&fos, 0, sizeof(SHFILEOPSTRUCT));

    fos.wFunc = FO_DELETE;
    fos.pFrom = dir.c_str();
    fos.fFlags = FOF_NO_UI;

    const auto result = ::SHFileOperation(&fos);
    if (result != NO_ERROR && result != ERROR_FILE_NOT_FOUND) {
        debugMessage(_T("Windscribe Installer - deleteFolder SHFileOperation failed: %d"), result);
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

    const auto isAdmin = isElevated();
    if (!isAdmin.has_value()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Couldn't determine if user has administrative rights."));
        return -1;
    }

    std::filesystem::path installPath;
    auto exitGuard = wsl::wsScopeGuard([&] {
        deleteFolder(installPath);
    });

    if (!isAdmin.value()) {
        if (!isOSCompatible()) {
            auto response = showMessageBox(NULL, _T("Windscribe Installer"), MB_YESNO | MB_ICONWARNING,
                                           _T("The Windscribe app may not function correctly on this version of Windows.  It requires Windows 10"
                                              " build %lu or newer for full functionality.\n\nDo you want to proceed with the install?"), kMinWindowsBuildNumber);
            if (response != IDYES) {
                return -1;
            }
        }

        // Attempt to run this program as admin
        TCHAR buf[MAX_PATH];
        DWORD result = GetModuleFileName(NULL, buf, MAX_PATH);
        if (result == 0 || result == MAX_PATH) {
            // 0 == failure; MAX_PATH == truncated (GetLastError() == ERROR_INSUFFICIENT_BUFFER) - reject either.
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't get own exe path."));
            return -1;
        }

        result = executeProgram(buf, lpszCmdParam, false, true, nullptr);
        if (result == NO_ERROR) {
            // Elevated process is running, exit this process
            return 0;
        } else if (result == ERROR_CANCELLED) {
            // User declined the UAC prompt; the installer cannot proceed without elevation, and we
            // intentionally do not extract anything to a Users-writable temp directory in this case.
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("The Windscribe installer requires administrator privileges to run."));
            return -1;
        } else {
            // Some other error occurred launching the elevated process
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't run installer as administrator."));
            return -1;
        }
    }

    // Elevated path: place the installer under C:\Windows\Temp in an Administrators+SYSTEM-only directory.
    auto randomSuffix = wsl::generateRandomSuffix();
    if (!randomSuffix) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to generate a secure random suffix for the installer directory."));
        return -1;
    }

    {
        wchar_t *windowsPath = NULL;
        auto memFree = wsl::wsScopeGuard([&] {
            ::CoTaskMemFree(windowsPath);
        });

        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Windows, 0, NULL, &windowsPath);
        if (FAILED(hr)) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't locate Windows directory (%lu)"), HRESULT_CODE(hr));
            return -1;
        }

        installPath = windowsPath;
        installPath.append(L"Temp");
        installPath.append(L"WindscribeInstaller" + *randomSuffix);

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
    auto result = executeProgram(app.c_str(), lpszCmdParam, true, isAdmin.value(), &exitCode);
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
