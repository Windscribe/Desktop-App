#include "copy_and_run.h"

#include <shlobj.h>
#include <shlwapi.h>

#include <filesystem>
#include <sstream>
#include <spdlog/spdlog.h>

#include "remove_directory.h"
#include "../utils/applicationinfo.h"
#include "../utils/secure_dir.h"
#include "../utils/utils.h"
#include "wsscopeguard.h"

unsigned long CopyAndRun::runFirstPhase(const std::wstring& uninstExeFile, const char *lpszCmdParam)
{
    // Use GetWindowsDirectory rather than SHGetKnownFolderPath so that no shell32 code is loaded
    // into this process, keeping it compatible with the Microsoft-signed-only load policy.
    wchar_t windowsPath[MAX_PATH];
    const UINT len = ::GetWindowsDirectory(windowsPath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        spdlog::error(L"GetWindowsDirectory failed to locate Windows directory ({})", ::GetLastError());
        return MAXDWORD;
    }

    auto randomSuffix = wsl::generateRandomSuffix();
    if (!randomSuffix) {
        spdlog::error(L"Failed to generate secure random suffix for uninstaller directory.");
        return MAXDWORD;
    }

    std::filesystem::path tempUninstallerPath(windowsPath);
    tempUninstallerPath.append(L"Temp");
    tempUninstallerPath.append(L"WindscribeUninstaller" + *randomSuffix);

    // Atomically create the uninstaller temp directory with a DACL granting access to Administrators
    // and SYSTEM only, so there is no window in which a lower-privileged process can place files
    // (including DLLs that the spawned uninstaller might load from its working directory) inside it.
    spdlog::info(L"Creating temporary uninstaller directory '{}'", tempUninstallerPath.c_str());
    auto logFn = [](const std::wstring &msg) {
        spdlog::error(L"{}", msg);
    };
    DWORD createErr = wsl::createAdminOnlyDirectory(tempUninstallerPath, logFn);
    if (createErr != ERROR_SUCCESS) {
        spdlog::error(L"Failed to create uninstaller directory '{}' ({})", tempUninstallerPath.c_str(), createErr);
        return MAXDWORD;
    }

    auto deleteTempFolderOnError = wsl::wsScopeGuard([&] {
        if (!tempUninstallerPath.empty()) {
            RemoveDir::DelTree(tempUninstallerPath.c_str(), true, true, true, false);
        }
    });

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
