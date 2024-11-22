#include "uninstallprev.h"

#include <QDeadlineTimer>
#include <QSettings>

#include <shlobj_core.h>
#include <windows.h>
#include <spdlog/spdlog.h>

#include "../installer_base.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/archive.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"
#include "global_consts.h"
#include "servicecontrolmanager.h"
#include "win32handle.h"

using namespace std;

UninstallPrev::UninstallPrev(bool isFactoryReset, double weight) : IInstallBlock(weight, L"UninstallPrev"),
    state_(0), isFactoryReset_(isFactoryReset)
{
}

int UninstallPrev::executeStep()
{
    if (state_ == 0) {
        HWND hwnd = Utils::appMainWindowHandle();

        if (hwnd) {
            spdlog::info(L"Windscribe is running - activating app so we can close it");
            // PostMessage WM_CLOSE will only work on an active window
            UINT dwActivateMessage = RegisterWindowMessage(L"WindscribeAppActivate");
            PostMessage(hwnd, dwActivateMessage, 0, 0);
        }

        QDeadlineTimer timerAppExit(10000);
        while (hwnd) {
            if (!timerAppExit.hasExpired()) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                Sleep(100);
                hwnd = Utils::appMainWindowHandle();
            }
            else {
                spdlog::warn(L"Timeout exceeded when trying to close Windscribe. Killing the process.");
                int result = taskKill(ApplicationInfo::appExeName());
                if (result != NO_ERROR) {
                    return result;
                }

                break;
            }
        }

        Sleep(1000);

        // We may have had to kill the app above, or may be in the situation where the app has
        // crashed, leaving a protocol handler (openvpn/WG) still running.  For example, we've
        // received installer logs indicating the app was not running, or exited before the
        // deadline given above, but the WG or openvpn binaries were still running, causing
        // the file installation phase to fail with 'file in use' errors.
        // The uninstaller we invoke in uninstallOldVersion() would typically provide this
        // functionality, but the user may have an older version of the app lacking this
        // uninstaller functionality.
        terminateProtocolHandlers();
        state_++;
        return 30;
    } else if (state_ == 1) {
        stopService();
        state_++;
        return 60;
    } else if (state_ == 2) {
        if (isFactoryReset_) {
            doFactoryReset();
        }

        wstring uninstallString = getUninstallString();
        if (!uninstallString.empty()) {
            spdlog::info(L"Found a previous installation of the application: {}", getPrevClientVersion());

            if (!isFactoryReset_) {
                QSettings reg(QString::fromStdWString(ApplicationInfo::appRegistryKey()), QSettings::NativeFormat);
                reg.setValue("userId", "");
            }

            DWORD lastError = 0;
            if (!uninstallOldVersion(uninstallString, lastError)) {
                spdlog::error(L"UninstallPrev::executeStep: uninstallOldVersion failed: {}", lastError);
                if (lastError != 2) { // Any error other than "Not found"
                    return -ERROR_OTHER;
                }

                // Uninstall failed because the uninstaller doesn't exist.
                if (!isPrevInstall64Bit()) {
                    return -ERROR_UNINSTALL;
                }

                if (!extractUninstaller()) {
                    spdlog::error(L"UninstallPrev::executeStep: could not extract uninstaller.");
                    return -ERROR_OTHER;
                }
                spdlog::info(L"UninstallPrev::executeStep: successfully extracted uninstaller, trying again.");
                return 65;
            }
        }
        return 100;
    }
    return 100;
}

wstring UninstallPrev::getUninstallString() const
{
    wstring uninstallString;

    QSettings reg(QString::fromStdWString(ApplicationInfo::uninstallerRegistryKey()), QSettings::NativeFormat);
    if (reg.contains(L"UninstallString")) {
        uninstallString = reg.value(L"UninstallString").toString().toStdWString();
        return uninstallString;
    }

    // 64-bit registry key not found, try 32-bit
    QSettings reg32(QString::fromStdWString(ApplicationInfo::uninstallerRegistryKey()), QSettings::Registry32Format);
    if (reg32.contains(L"UninstallString")) {
        uninstallString = reg32.value(L"UninstallString").toString().toStdWString();
    }

    return uninstallString;
}

bool UninstallPrev::isPrevInstall64Bit() const
{
    QSettings reg(QString::fromStdWString(ApplicationInfo::uninstallerRegistryKey()), QSettings::NativeFormat);
    if (reg.contains(L"UninstallString")) {
        return true;
    }
    return false;
}

bool UninstallPrev::uninstallOldVersion(const wstring &uninstallString, DWORD &lastError) const
{
    const wstring sUnInstallString = removeQuotes(uninstallString);
    DWORD error = 0;

    const auto res = Utils::instExec(sUnInstallString, L"/VERYSILENT", INFINITE, SW_HIDE, L"", &error);
    if (!res.has_value() || res.value() == MAXDWORD) {
        lastError = error;
        return false;
    }

    // Uninstall process can return second phase processId, which we need wait to finish.  We'll be optimistic
    // here and assume all is okay to proceed if a warning is logged below during this phase.
    if (res.value() != 0) {
        wsl::Win32Handle processHandle(::OpenProcess(SYNCHRONIZE, FALSE, res.value()));
        if (processHandle.isValid()) {
            if (processHandle.wait(INFINITE) != WAIT_OBJECT_0) {
                spdlog::warn(L"WARNING: wait for second phase uninstall failed, uninstaller may have crashed.");
            }
        } else {
            spdlog::warn(L"WARNING: unable to obtain process handle for second phase uninstaller.");
        }
    }

    return true;
}

wstring UninstallPrev::removeQuotes(const wstring &str) const
{
    wstring str1 = str;

    while ((!str1.empty()) && (str1[0] == '"' || str1[0] == '\'')) {
        str1.erase(0, 1);
    }

    while ((!str1.empty()) && (str1[str1.length() - 1] == '"' || str1[str1.length() - 1] == '\'')) {
        str1.erase(str1.length() - 1, 1);
    }

    return str1;
}

void UninstallPrev::doFactoryReset() const
{
    // Delete preferences
    QSettings reg(QString::fromStdWString(ApplicationInfo::appRegistryKey()), QSettings::NativeFormat);
    reg.remove("");

    // Delete logs and other data
    // NB: Does not delete any files in use, such as the uninstaller log
    wchar_t* localAppData = nullptr;

    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppData) == S_OK) {
        wstring deletePath(localAppData);
        deletePath.append(L"\\windscribe_extra.conf");
        DeleteFile(deletePath.c_str());

        deletePath = localAppData;
        CoTaskMemFree(static_cast<void*>(localAppData));
        deletePath.append(L"\\Windscribe\\Windscribe2\\*");

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(deletePath.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                wstring fullPath(deletePath);
                fullPath.erase(fullPath.size()-1); // erase '*'
                fullPath.append(ffd.cFileName);
                DeleteFile(fullPath.c_str());
            }
            while (FindNextFile(hFind, &ffd) != 0);
            FindClose(hFind);
        }
        DeleteFile(deletePath.c_str());
    }
}

void UninstallPrev::stopService() const
{
    try {
        wsl::ServiceControlManager scm;
        scm.stopService(ApplicationInfo::serviceName().c_str());
    }
    catch (system_error& ex) {
        spdlog::error("UninstallPrev::stopService {} ({})", ex.what(), ex.code().value());
    }
}

bool UninstallPrev::extractUninstaller() const
{
    spdlog::info(L"Extracting uninstaller from the archive");

    wsl::Archive archive;
    archive.setLogFunction([](const wstring &str) {
        spdlog::info(L"{}", str);
    });

    const wstring exePath = Utils::getExePath();
    if (exePath.empty()) {
        spdlog::error(L"Could not get exe path");
        return false;
    }

    const wstring targetFolder = Path::extractDir(removeQuotes(getUninstallString()));

    if (!archive.extract(L"Windscribe", L"windscribe.7z", exePath, L"uninstall.exe", targetFolder)) {
        return false;
    }

    return true;
}

void UninstallPrev::terminateProtocolHandlers() const
{
    // We'll be optimistic here and proceed with the install even if one of these steps
    // fails.  The uninstaller, unless it is a very old one, will take another stab at it.

    try {
        wsl::ServiceControlManager scm;
        scm.stopService(kWireGuardServiceIdentifier.c_str());
    }
    catch (std::system_error& ex) {
        spdlog::warn("WARNING: failed to stop the WireGuard service - {}", ex.what());
    }

    taskKill(L"windscribeopenvpn.exe");
    taskKill(L"windscribewstunnel.exe");
    taskKill(L"windscribectrld.exe");
}

wstring UninstallPrev::getPrevClientVersion() const
{
    // Read the path of the installed client from the registry and determine Windscribe.exe version
    QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);
    if (!reg.contains("applicationPath"))
        return wstring();

    const auto path = reg.value("applicationPath").toString().toStdWString() + L"\\Windscribe.exe";

    DWORD dwSize = GetFileVersionInfoSize(path.c_str(), NULL);
    if (dwSize == 0) {
        spdlog::error("Error in GetFileVersionInfoSize: {}", GetLastError());
        return wstring();
    }

    std::vector<BYTE> pVersionInfo(dwSize);
    if (!GetFileVersionInfo(path.c_str(), 0, dwSize, pVersionInfo.data())) {
        spdlog::error("Error in GetFileVersionInfo: {}", GetLastError());
        return wstring();
    }

    VS_FIXEDFILEINFO    *pFileInfo          = NULL;
    UINT                pLenFileInfo        = 0;
    if (!VerQueryValue(pVersionInfo.data(), TEXT("\\"), (LPVOID*) &pFileInfo, &pLenFileInfo)) {
        spdlog::error("Error in VerQueryValue: {}", GetLastError());
        return wstring();
    }

    DWORD major  =  ( pFileInfo->dwFileVersionMS >> 16 ) & 0xffff ;
    DWORD minor  =  ( pFileInfo->dwFileVersionMS) & 0xffff;
    DWORD build =  ( pFileInfo->dwFileVersionLS >>  16 ) & 0xffff;

    return fmt::format(L"v{}.{}.{}", major, minor, build);
}

int UninstallPrev::taskKill(const std::wstring &exeName) const
{
    const wstring appName = Path::append(Utils::getSystemDir(), L"taskkill.exe");
    const wstring commandLine = L"/f /t /im " + exeName;
    const auto result = Utils::instExec(appName, commandLine, INFINITE, SW_HIDE);
    if (!result.has_value()) {
        spdlog::warn(L"WARNING: an error was encountered attempting to start taskkill.exe.");
        return -ERROR_OTHER;
    }

    if (result.value() != NO_ERROR && result.value() != ERROR_WAIT_NO_CHILDREN) {
        spdlog::warn(L"WARNING: unable to kill {} ({}).", exeName, result.value());
        return -ERROR_OTHER;
    }

    spdlog::info(L"taskkill executed on {} ({})", exeName, result.value());
    return NO_ERROR;
}
