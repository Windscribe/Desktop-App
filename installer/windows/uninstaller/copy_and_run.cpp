#include "copy_and_run.h"

#include <aclapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <filesystem>
#include <sstream>
#include <spdlog/spdlog.h>

#include "remove_directory.h"
#include "../utils/applicationinfo.h"
#include "../utils/utils.h"
#include "wsscopeguard.h"

static bool setFolderPermissions(const std::filesystem::path &folder)
{
    SECURITY_DESCRIPTOR sd;
    ::ZeroMemory(&sd, sizeof(sd));

    BOOL result = ::InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (!result) {
        spdlog::error(L"setFolderPermissions InitializeSecurityDescriptor failed: {}", ::GetLastError());
        return false;
    }

    // Create a SID for the BUILTIN\Administrators group
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID sidAdmin = NULL;
    result = ::AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &sidAdmin);
    if (!result) {
        spdlog::error(L"setFolderPermissions AllocateAndInitializeSid failed: {}", ::GetLastError());
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
        spdlog::error(L"setFolderPermissions SetEntriesInAcl failed: {}", result);
        return false;
    }

    result = ::SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE);
    if (!result) {
        spdlog::error(L"setFolderPermissions SetSecurityDescriptorDacl failed: {}", ::GetLastError());
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
        spdlog::error(L"setFolderPermissions SetFileSecurity failed: {}", ::GetLastError());
        return false;
    }

    return true;
}

unsigned long CopyAndRun::runFirstPhase(const std::wstring& uninstExeFile, const char *lpszCmdParam)
{
    wchar_t *windowsPath = NULL;
    HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_Windows, 0, NULL, &windowsPath);
    if (FAILED(hr)) {
        spdlog::error(L"SHGetKnownFolderPath failed to locate Windows directory: {}", HRESULT_CODE(hr));
        return MAXDWORD;
    }

    srand(time(NULL));
    std::filesystem::path tempUninstallerPath(windowsPath);
    ::CoTaskMemFree(windowsPath);
    tempUninstallerPath.append(L"Temp");
    tempUninstallerPath.append(L"WindscribeUninstaller" + std::to_wstring(rand()));

    std::error_code ec;
    if (std::filesystem::exists(tempUninstallerPath, ec)) {
        spdlog::error(L"The randomly selected temporary uninstall folder already exists.  This is highly unusual and may indicate"
                      L" an attack on your system.  The uninstaller is aborting to protect the integrity of your system."
                      L" The folder is: {}", tempUninstallerPath.c_str());
        return MAXDWORD;
    } else if (ec) {
        spdlog::warn("Windscribe uninstaller failed to check if temp uninstall folder exists ({})", ec.message());
    }

    // Create the target directory and set its permissions to restrict access to only members of the
    // built-in Administrators group.
    spdlog::info(L"Creating temporary uninstaller directory '{}'", tempUninstallerPath.c_str());
    std::filesystem::create_directories(tempUninstallerPath, ec);
    if (ec) {
        spdlog::error("Failed to create the temporary uninstaller directory: {}", ec.message());
        return MAXDWORD;
    }

    auto deleteTempFolderOnError = wsl::wsScopeGuard([&] {
        if (!tempUninstallerPath.empty()) {
            RemoveDir::DelTree(tempUninstallerPath.c_str(), true, true, true, false);
        }
    });

    if (!setFolderPermissions(tempUninstallerPath)) {
        return MAXDWORD;
    }

    std::filesystem::path tempUninstaller(tempUninstallerPath);
    tempUninstaller.append(ApplicationInfo::uninstaller());

    // We should have created a unique folder, thus we want CopyFile to fail if the file already exists.
    BOOL result = ::CopyFile(uninstExeFile.c_str(), tempUninstaller.c_str(), TRUE);
    if (result == FALSE) {
        spdlog::error(L"CopyAndRun::runFirstPhase failed to copy [{}] to [{}] ({})", uninstExeFile, tempUninstaller.c_str(), ::GetLastError());
        return MAXDWORD;
    }

    // Don't want any attribute like read-only transferred
    ::SetFileAttributes(tempUninstaller.c_str(), FILE_ATTRIBUTE_NORMAL);

    // As per the Win32 docs; the Unicode version of CreateProcess 'may' modify its lpCommandLine
    // parameter.  Therefore, this parameter cannot be a pointer to read-only memory (such as a
    // const variable or a literal string).  If this parameter is a constant string, CreateProcess
    // may cause an access violation.  Maximum length of the lpCommandLine parameter is 32767.
    std::unique_ptr<wchar_t[]> cmdLine(new wchar_t[32767]);
    {
        std::wostringstream stream;
        stream << L"\"" << tempUninstaller.c_str() << L"\" /SECONDPHASE=\"" << uninstExeFile << L"\" " << lpszCmdParam;
        wcsncpy_s(cmdLine.get(), 32767, stream.str().c_str(), _TRUNCATE);
    }

    spdlog::info(L"Starting uninstaller process: {}", cmdLine.get());

    STARTUPINFO startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    // Execute the copy of itself ("second phase")
    result = ::CreateProcess(nullptr, cmdLine.get(), nullptr, nullptr, false, 0, nullptr,
                             tempUninstallerPath.c_str(), &startupInfo, &processInfo);
    if (result == FALSE) {
        spdlog::error(L"CopyAndRun::exec - CreateProcess({}) failed ({})", cmdLine.get(), ::GetLastError());
        return MAXDWORD;
    }

    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(processInfo.hProcess);

    deleteTempFolderOnError.dismiss();

    // Mark the uninstaller for deletion, then the folder, as this method will fail to remove a non-empty folder.
    Utils::deleteFileOnReboot(tempUninstaller);
    Utils::deleteFileOnReboot(tempUninstallerPath);

    return processInfo.dwProcessId;
}
