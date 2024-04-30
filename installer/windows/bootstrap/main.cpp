#include <Windows.h>

#include <fileapi.h>
#include <filesystem>
#include <optional>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <string>
#include <tchar.h>
#include <versionhelpers.h>

#include "../../../client/common/archive/archive.h"
#include "global_consts.h"
#include "wsscopeguard.h"
#include "win32handle.h"

// Set the DLL load directory to the system directory before entering WinMain().
struct LoadSystemDLLsFromSystem32
{
    LoadSystemDLLsFromSystem32()
    {
        // Remove the current directory from the search path for dynamically loaded
        // DLLs as a precaution.  This call has no effect for delay load DLLs.
        SetDllDirectory(L"");
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
} loadSystemDLLs;

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

static DWORD ExecuteProgram(const wchar_t *cmd, const wchar_t *params, bool wait, bool asAdmin, LPDWORD exitCode)
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

static void DeleteFolder(const wchar_t *folder)
{
    // SHFileOperation requires file names to be terminated with two \0s
    std::wstring dir = folder;
    dir += L'\0';

    SHFILEOPSTRUCT fos;
    memset(&fos, 0, sizeof(SHFILEOPSTRUCT));

    fos.wFunc = FO_DELETE;
    fos.pFrom = dir.c_str();
    fos.fFlags = FOF_NO_UI;

    SHFileOperation(&fos);
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

    auto isAdmin = isElevated();
    if (!isAdmin.has_value()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Couldn't determine if user has administrative rights."));
        return -1;
    }

    std::wstringstream path;
    srand(time(NULL)); // Doesn't have to be cryptographically secure, just random

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
        if (result == NO_ERROR) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't get own exe path."));
            return -1;
        }
        result = ExecuteProgram(buf, lpszCmdParam, false, true, nullptr);
        if (result == NO_ERROR) {
            // Elevated process is running, exit this process
            return 0;
        } else if (result != ERROR_CANCELLED) {
            // Some error occurred launching the elevated process
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't run installer as administrator."));
            return -1;
        }

        // Otherwise, UAC prompt was cancelled, continue unelevated so we can show the error

        // Find the temporary dir
        wchar_t tempPath[MAX_PATH];
        result = GetTempPath(MAX_PATH, tempPath);
        if (result == 0) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't locate Windows temporary directory (%lu)"), GetLastError());
            return -1;
        }
        path << tempPath;
        path << L"\\WindscribeInstaller" + std::to_wstring(rand());
    } else {
        // Find the Windows dir
        wchar_t *windowsPath = NULL;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Windows, 0, NULL, &windowsPath);
        if (FAILED(hr)) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Couldn't locate Windows directory (%lu)"), HRESULT_CODE(hr));
            CoTaskMemFree(windowsPath);
            return -1;
        }
        path << windowsPath;
        CoTaskMemFree(windowsPath);
        path << L"\\Temp\\WindscribeInstaller" + std::to_wstring(rand());
    }

    std::wstring installPath = path.str();

    auto exitGuard = wsl::wsScopeGuard([&]
    {
        // Delete the temporary folder
        DeleteFolder(installPath.c_str());
    });

    // Extract archive
    std::wstring archive = L"Installer";
    std::unique_ptr<Archive> a(new Archive(archive));

    std::list<std::wstring> fileList;
    std::list<std::wstring> pathList;

    SRes res = a->fileList(fileList);
    if (res != SZ_OK) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
            _T("Couldn't get archive file list"));
        return -1;
    }

    pathList.clear();
    for (auto it = fileList.cbegin(); it != fileList.cend(); it++) {
        std::filesystem::path p(installPath + L"\\" + *it);
        pathList.push_back(p.parent_path());
    }

    a->calcTotal(fileList, pathList);
    for (int fileIndex = 0; fileIndex < a->getNumFiles(); fileIndex++) {
        SRes res = a->extractionFile(fileIndex);
        if (res != SZ_OK) {
            a->finish();
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                _T("Couldn't extract files"));
            return -1;
        }

    }
    a->finish();

    std::wstring app = installPath + L"\\";
    app.append(WINDSCRIBE_INSTALLER_NAME);

    DWORD exitCode = 0;
    auto result = ExecuteProgram(app.c_str(), lpszCmdParam, true, isAdmin.value(), &exitCode);
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
