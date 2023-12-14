#include <Windows.h>

#include <fileapi.h>
#include <filesystem>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <tchar.h>
#include <iostream>
#include <versionhelpers.h>

#include "archive/archive.h"
#include "wsscopeguard.h"

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

static constexpr DWORD kMinOSBuildNumber = 17763;

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

static DWORD ExecuteProgram(const wchar_t *cmd, const wchar_t *params, bool wait, bool asAdmin)
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

    if (wait) {
        if (ei.hProcess != 0) {
            WaitForSingleObject(ei.hProcess, INFINITE);
            CloseHandle(ei.hProcess);
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
    if (!IsWindows10OrGreater()) {
        return false;
    }

    // The version of Qt we use, and some Win32 functions we call, require a minimum of Windows 10 1809.
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
    return (rtlOsVer.dwBuildNumber >= kMinOSBuildNumber);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpszCmdParam, int nCmdShow)
{
    // We make this check here, rather than in the main installer, since it may fail to launch on an
    // incompatible OS due to missing function imports.
    if (!isOSCompatible()) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("The Windscribe app cannot be installed on this version of Windows.  It requires Windows 10 build %lu or newer."), kMinOSBuildNumber);
        return -1;
    }

    // Find the temporary dir
    wchar_t path[MAX_PATH];
    DWORD result = GetTempPath(MAX_PATH, path);
    if (result == 0) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Couldn't locate Windows temporary directory (%lu)"), GetLastError());
        return -1;
    }

    // Create install path
    std::wstring installPath = path;
    srand(time(NULL)); // Doesn't have to be cryptographically secure, just random
    installPath += L"\\WindscribeBootstrap" + std::to_wstring(rand());

    if (::SHCreateDirectoryEx(NULL, installPath.c_str(), NULL) != ERROR_SUCCESS) {
        if (::GetLastError() != ERROR_ALREADY_EXISTS) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                _T("Couldn't create installer temporary directory"));
            return -1;
        }
    }

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

    // Run the installer as admin.  If the user rejects the UAC prompt, run the installer unelevated
    // so it can display a proper UI to the user explaining that elevation is required.
    result = ExecuteProgram(app.c_str(), lpszCmdParam, true, true);
    if (result == ERROR_CANCELLED) {
        result = ExecuteProgram(app.c_str(), lpszCmdParam, true, false);
    }

    if (result != NO_ERROR) {
        showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                       _T("Windows was unable to launch the installer (%lu)"), result);
        return -1;
    }

    return 0;
}
