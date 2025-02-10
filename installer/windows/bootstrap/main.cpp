#include <Windows.h>

#include <aclapi.h>
#include <fileapi.h>
#include <filesystem>
#include <optional>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <tchar.h>
#include <versionhelpers.h>

#include "../utils/archive.h"
#include "global_consts.h"
#include "wsscopeguard.h"
#include "win32handle.h"

#include "../../../libs/wssecure/wssecure_globals.h"

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

static bool setFolderPermissions(const std::filesystem::path &folder)
{
    SECURITY_DESCRIPTOR sd;
    ::ZeroMemory(&sd, sizeof(sd));

    BOOL result = ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (!result) {
        debugMessage(_T("setFolderPermissions InitializeSecurityDescriptor failed: %lu"), ::GetLastError());
        return false;
    }

    // Create a SID for the BUILTIN\Administrators group
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID sidAdmin = NULL;
    result = ::AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sidAdmin);
    if (!result) {
        debugMessage(_T("setFolderPermissions AllocateAndInitializeSid failed: %lu"), ::GetLastError());
        return false;
    }

    PACL pACL = NULL;
    auto exitGuard = wsl::wsScopeGuard([&] {
        if (pACL) {
            ::LocalFree(pACL);
        }

        if (sidAdmin) {
            ::FreeSid(sidAdmin);
        }
    });

    // Set up an EXPLICIT_ACCESS structure for the Administrators group.
    EXPLICIT_ACCESS ea;
    ::ZeroMemory(&ea, sizeof(ea));
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName = reinterpret_cast<LPTSTR>(sidAdmin);

    // Create a new ACL with these permissions.
    result = ::SetEntriesInAcl(1, &ea, NULL, &pACL);
    if (result != ERROR_SUCCESS) {
        debugMessage(_T("setFolderPermissions SetEntriesInAcl failed: %lu"), result);
        return false;
    }

    result = ::SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE);
    if (!result) {
        debugMessage(_T("setFolderPermissions SetSecurityDescriptorDacl failed: %lu"), ::GetLastError());
        return false;
    }

    SECURITY_ATTRIBUTES sa;
    ::ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    // Apply the security descriptor to the directory
    result = ::SetFileSecurity(folder.c_str(), DACL_SECURITY_INFORMATION, &sd);
    if (!result) {
        debugMessage(_T("setFolderPermissions SetFileSecurity failed: %lu"), ::GetLastError());
        return false;
    }

    return true;
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

        result = executeProgram(buf, lpszCmdParam, false, true, nullptr);
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
        installPath = tempPath;
        installPath.append(L"WindscribeInstaller" + std::to_wstring(rand()));
    } else {
        // Find the Windows dir
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
        installPath.append(L"WindscribeInstaller" + std::to_wstring(rand()));

        std::error_code ec;
        if (std::filesystem::exists(installPath, ec)) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("The randomly selected temporary install folder already exists.  This is highly unusual and may indicate")
                           _T(" an attack on your system.  The installer is aborting to protect the integrity of your system.")
                           _T("\n\nThe folder is: %s"), installPath.c_str());
            return -1;
        } else if (ec) {
            debugMessage(L"Windscribe bootstrapper failed to check if temp install folder exists (%hs)", ec.message().c_str());
        }

        // Create the target directory and set its permissions to restrict access to only members of the
        // built-in Administrators group.
        std::filesystem::create_directories(installPath, ec);
        if (ec) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Failed to create the installer directory '%s' (%hs)"), installPath.c_str(), ec.message().c_str());
            return -1;
        }

        if (!setFolderPermissions(installPath)) {
            showMessageBox(NULL, _T("Windscribe Installer"), MB_OK | MB_ICONSTOP,
                           _T("Failed to set permissions on installer directory '%s'"), installPath.c_str());
            return -1;
        }
    }

    debugMessage(L"Windscribe bootstrapper installing to %s", installPath.c_str());

    // Extract archive
    wsl::Archive archive;
    archive.setLogFunction([](const std::wstring &str) {
        debugMessage(str.c_str());
    });

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
