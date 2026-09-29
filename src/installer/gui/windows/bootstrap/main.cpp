#include <Windows.h>
#include <sddl.h>

#include <cstdio>
#include <fileapi.h>
#include <filesystem>
#include <optional>
#include <string>
#include <tchar.h>
#include <versionhelpers.h>

#include "../utils/archive.h"
#include "../utils/persistent_log.h"
#include "../utils/secure_dir.h"
#include "global_consts.h"
#include "win32handle.h"
#include "wsprocessmitigations.h"
#include "wsscopeguard.h"

// Machine-wide (Global\) so two sessions cannot install at once.  Branded so a white-label
// build does not collide with Windscribe's mutex.
static constexpr wchar_t kInstallerMutexName[] = L"Global\\" WS_APP_IDENTIFIER_W L"Installer";

// Set from -silent before any UI.  Store/Intune/SCCM launches have no user to dismiss a dialog.
static bool silentInstall = false;

// Returns ERROR_SUCCESS and writes *elevated, or a Win32 error if the process
// token could not be queried.  GetLastError is captured before Win32Handle
// runs CloseHandle, which would overwrite it.
static DWORD checkElevation(bool *elevated)
{
    wsl::Win32Handle token;
    BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.data());
    if (result == FALSE) {
        return ::GetLastError();
    }

    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof(TOKEN_ELEVATION);
    result = GetTokenInformation(token.getHandle(), TokenElevation, &Elevation, sizeof(Elevation), &cbSize);
    if (result == FALSE) {
        return ::GetLastError();
    }

    *elevated = Elevation.TokenIsElevated != FALSE;
    return ERROR_SUCCESS;
}

static int showMessageBox(HWND hOwner, LPCTSTR szTitle, UINT nStyle, LPCTSTR szFormat, ...)
{
    va_list arg_list;
    va_start(arg_list, szFormat);

    TCHAR Buf[1024];
    _vsntprintf_s(Buf, 1024, _TRUNCATE, szFormat, arg_list);
    va_end(arg_list);

    if (silentInstall) {
        ::OutputDebugString(Buf);
        return 0;
    }

    return ::MessageBox(hOwner, Buf, szTitle, nStyle);
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

// Writes an entry to the shared %ProgramData%\Windscribe\installer.log in the same JSON
// format the installer produces (log_utils::createJsonFormatter), so one install attempt
// reads as a single chronological log: bootstrap prelude, then the installer's entries,
// then the bootstrap's exit-code epilogue.
static void logToFile(wsl::PersistentLog &log, const char *level, const std::wstring &msg)
{
    if (!log.isOpen()) {
        return;
    }

    std::string utf8;
    if (!msg.empty()) {
        const int size = ::WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.size()), nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            utf8.resize(size);
            ::WideCharToMultiByte(CP_UTF8, 0, msg.c_str(), static_cast<int>(msg.size()), utf8.data(), size, nullptr, nullptr);
        }
    }

    std::string escaped;
    escaped.reserve(utf8.size());
    for (char c : utf8) {
        switch (c) {
        case '"':  escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n";  break;
        case '\r': escaped += "\\r";  break;
        case '\t': escaped += "\\t";  break;
        default:
            escaped += (static_cast<unsigned char>(c) < 0x20) ? ' ' : c;
        }
    }

    SYSTEMTIME st;
    ::GetLocalTime(&st);

    char header[128];
    snprintf(header, sizeof(header),
             "{\"tm\": \"%04u-%02u-%02u %02u:%02u:%02u.%03u\", \"lvl\": \"%s\", \"mod\": \"bootstrap\", \"msg\": \"",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, level);

    log.write(header + escaped + "\"}\n");
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
            *exitCode = ::GetLastError();
            if (*exitCode == ERROR_SUCCESS) {
                *exitCode = ERROR_GEN_FAILURE;
            }
            debugMessage(_T("Windscribe Installer - GetExitCodeProcess failed: %lu"), *exitCode);
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

// Administrators + SYSTEM only, so a non-elevated process cannot squat the name and block
// installation.  ConvertStringSecurityDescriptorToSecurityDescriptorW is in Microsoft-signed
// advapi32, so it is compatible with this process's Microsoft-signed-only load policy.
static DWORD createInstallerMutexSecurityAttributes(SECURITY_ATTRIBUTES *sa)
{
    sa->nLength = sizeof(SECURITY_ATTRIBUTES);
    sa->bInheritHandle = FALSE;
    sa->lpSecurityDescriptor = nullptr;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:P(A;;GA;;;SY)(A;;GA;;;BA)", SDDL_REVISION_1, &sa->lpSecurityDescriptor, nullptr)) {
        return ::GetLastError();
    }
    return ERROR_SUCCESS;
}

// Creates (or opens) the single-instance mutex and stores the handle in mutex, which must
// remain alive for the rest of this process so the inner installer is covered too.
// ERROR_SUCCESS: this process now holds the only instance.
// ERROR_INSTALL_ALREADY_RUNNING: another instance already holds it.
// Any other Win32 code: the mutex could not be created.
static DWORD acquireSingleInstanceMutex(wsl::Win32Handle &mutex)
{
    SECURITY_ATTRIBUTES sa;
    const DWORD sdErr = createInstallerMutexSecurityAttributes(&sa);
    if (sdErr != ERROR_SUCCESS) {
        return sdErr;
    }
    auto freeSd = wsl::wsScopeGuard([&] {
        if (sa.lpSecurityDescriptor) {
            ::LocalFree(sa.lpSecurityDescriptor);
        }
    });

    HANDLE handle = ::CreateMutexW(&sa, FALSE, kInstallerMutexName);
    const DWORD err = ::GetLastError();
    if (handle == nullptr) {
        return err;
    }

    mutex.setHandle(handle);
    if (err == ERROR_ALREADY_EXISTS) {
        mutex.closeHandle();
        return ERROR_INSTALL_ALREADY_RUNNING;
    }
    return ERROR_SUCCESS;
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

// True when cmdParam contains a standalone -silent token.  Parsed without CommandLineToArgvW
// so this process does not load shell32 (see launchAndWait).
static bool hasSilentFlag(LPCWSTR cmdParam)
{
    if (cmdParam == nullptr) {
        return false;
    }

    const wchar_t *p = cmdParam;
    while (*p) {
        while (*p == L' ' || *p == L'\t') {
            ++p;
        }
        if (*p == L'\0') {
            break;
        }

        const wchar_t *tokenStart;
        const wchar_t *tokenEnd;
        if (*p == L'"') {
            ++p;
            tokenStart = p;
            while (*p && *p != L'"') {
                ++p;
            }
            tokenEnd = p;
            if (*p == L'"') {
                ++p;
            }
        } else {
            tokenStart = p;
            while (*p && *p != L' ' && *p != L'\t') {
                ++p;
            }
            tokenEnd = p;
        }

        if ((tokenEnd - tokenStart) == 7 && wcsncmp(tokenStart, L"-silent", 7) == 0) {
            return true;
        }
    }
    return false;
}

// Process exit code is a Win32 error code: ERROR_SUCCESS on success, or a specific
// failure such as ERROR_CANCELLED, ERROR_ELEVATION_REQUIRED, ERROR_OLD_WIN_VERSION,
// ERROR_INSTALL_ALREADY_RUNNING, or ERROR_INSTALL_FAILURE.  Callers (Store, Intune,
// SCCM, scripts) can distinguish outcomes without parsing UI or logs.
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpszCmdParam, int nCmdShow)
{
    silentInstall = hasSilentFlag(lpszCmdParam);

    if (!IsWindows10OrGreater()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The Windscribe app cannot be installed on this version of Windows.  It requires Windows 10 or newer."));
        return ERROR_OLD_WIN_VERSION;
    }

    // The executable requests requireAdministrator, so the OS elevates this process at launch.  Verify elevation defensively.
    bool isAdmin = false;
    const DWORD elevationErr = checkElevation(&isAdmin);
    if (elevationErr != ERROR_SUCCESS) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Couldn't determine if user has administrative rights (%lu)."), elevationErr);
        return elevationErr;
    }
    if (!isAdmin) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The Windscribe installer requires administrator privileges to run."));
        return ERROR_ELEVATION_REQUIRED;
    }

    // Hold this handle until wWinMain returns so the mutex covers the inner installer that
    // launchAndWait waits on.  Taken before the log is opened so a second instance cannot
    // rotate the first instance's installer.log out from under it.
    wsl::Win32Handle instanceMutex;
    {
        const DWORD err = acquireSingleInstanceMutex(instanceMutex);
        if (err == ERROR_INSTALL_ALREADY_RUNNING) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Another Windscribe installer is already running.  Complete or close that")
                           _T(" installer before starting a new one."));
            return ERROR_INSTALL_ALREADY_RUNNING;
        }
        if (err != ERROR_SUCCESS) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Failed to create the installer single-instance mutex (%lu)."), err);
            return err;
        }
    }

    if (!isOSCompatible()) {
        if (silentInstall) {
            debugMessage(_T("Windscribe installer proceeding on a Windows build older than %lu (silent)"), kMinWindowsBuildNumber);
        } else {
            auto response = showMessageBox(NULL, _T("Windscribe Installer"), MB_YESNO | MB_ICONWARNING,
                                           _T("The Windscribe app may not function correctly on this version of Windows.  It requires Windows 10"
                                              " build %lu or newer for full functionality.\n\nDo you want to proceed with the install?"), kMinWindowsBuildNumber);
            if (response != IDYES) {
                return ERROR_CANCELLED;
            }
        }
    }

    // Start the shared persistent log: the bootstrap runs first and rotates the previous
    // attempt's log to installer.log.1; the child installer appends to the same file.
    // SHGetKnownFolderPath (used by PersistentLog) loads only Microsoft-signed shell32 and,
    // unlike ShellExecuteEx/SHFileOperation, does not invoke third-party shell-extension
    // DLLs, so it is compatible with the Microsoft-signed-only load policy of this process.
    wsl::PersistentLog persistentLog;
    persistentLog.open(L"installer.log", wsl::PersistentLog::OpenMode::Rotate);

    std::filesystem::path installPath;
    auto exitGuard = wsl::wsScopeGuard([&] {
        deleteFolder(installPath);
    });

    // Place the installer under C:\Windows\Temp in an Administrators+SYSTEM-only directory.
    auto randomSuffix = wsl::generateRandomSuffix();
    if (!randomSuffix) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to generate a secure random suffix for the installer directory."));
        return ERROR_GEN_FAILURE;
    }

    {
        // GetWindowsDirectory is sufficient here; shell32 folder-lookup calls are also safe
        // under the Microsoft-signed-only policy (see the PersistentLog note above), but
        // ShellExecuteEx/SHFileOperation-style calls that can pull in third-party
        // shell-extension DLLs must still be avoided (see launchAndWait).
        wchar_t windowsDir[MAX_PATH];
        const UINT len = ::GetWindowsDirectoryW(windowsDir, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            const DWORD err = (len == 0) ? ::GetLastError() : ERROR_INSUFFICIENT_BUFFER;
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't locate Windows directory (%lu)"), err);
            return err;
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
            return err;
        }
        if (err != ERROR_SUCCESS) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Failed to create the installer directory '%s' (%lu)."), installPath.c_str(), err);
            return err;
        }
    }

    debugMessage(L"Windscribe bootstrapper installing to %s", installPath.c_str());
    if (silentInstall) {
        logToFile(persistentLog, "info", L"Bootstrap running in silent mode");
    }
    logToFile(persistentLog, "info", L"Bootstrap staging the Windscribe installer to " + installPath.wstring());

    // Extract archive
    wsl::Archive archive;
    archive.setLogFunction([&persistentLog](const std::wstring &str) {
        debugMessage(str.c_str());
        logToFile(persistentLog, "info", str);
    });

    if (!archive.extractDecompressor(installPath)) {
        logToFile(persistentLog, "error", L"Bootstrap failed to extract the Windscribe decompressor");
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to extract the Windscribe decompressor."));
        return ERROR_INSTALL_FAILURE;
    }

    if (!archive.extract(L"Installer", L"windscribeinstaller.7z", installPath, installPath)) {
        logToFile(persistentLog, "error", L"Bootstrap failed to extract the Windscribe installer");
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Failed to extract the Windscribe installer."));
        return ERROR_INSTALL_FAILURE;
    }

    std::filesystem::path app(installPath);
    app.append(WINDSCRIBE_INSTALLER_NAME);

    logToFile(persistentLog, "info", L"Bootstrap launching " + app.wstring());

    DWORD exitCode = 0;
    auto result = launchAndWait(app.wstring(), lpszCmdParam, &exitCode);
    if (result != NO_ERROR) {
        logToFile(persistentLog, "error", L"Bootstrap was unable to launch the installer (" + std::to_wstring(result) + L")");
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Windows was unable to launch the installer (%lu)"), result);
        return result;
    }

    if (exitCode == ERROR_CANCELLED) {
        logToFile(persistentLog, "info", L"The installer was cancelled");
    } else if (exitCode != ERROR_SUCCESS) {
        debugMessage(_T("Windscribe installer exited with an error: %lu"), exitCode);
        wchar_t exitCodeMsg[128];
        // Win32 error codes fit in 16 bits.  Larger values are typically NTSTATUS / exception
        // codes from an abnormal termination (crash, abort), not a mapped installer error.
        if (exitCode > 0xFFFF) {
            swprintf_s(exitCodeMsg, L"The installer exited with an unexpected code %lu (0x%08X), indicating it may have crashed", exitCode, exitCode);
            logToFile(persistentLog, "error", exitCodeMsg);
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("The installer reported an unexpected exit code, indicating it may have crashed during exit.")
                           _T(" Please report this failure to Windscribe support."));
        } else {
            swprintf_s(exitCodeMsg, L"The installer exited with code %lu (0x%08X)", exitCode, exitCode);
            logToFile(persistentLog, "error", exitCodeMsg);
        }
    }

    return exitCode;
}
