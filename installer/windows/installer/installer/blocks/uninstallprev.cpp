#include "uninstallprev.h"

#include <QSettings>

#include <chrono>
#include <shlobj_core.h>
#include <windows.h>

#include "../installer_base.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/archive.h"
#include "../../../utils/logger.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"
#include "servicecontrolmanager.h"
#include "win32handle.h"

using namespace std;

const wstring wmActivateGui = L"WindscribeAppActivate";
const auto kWindscribeClosingTimeout = chrono::milliseconds(5000);


UninstallPrev::UninstallPrev(bool isFactoryReset, double weight) : IInstallBlock(weight, L"UninstallPrev"),
    state_(0), isFactoryReset_(isFactoryReset)
{
}

int UninstallPrev::executeStep()
{
    namespace chr = chrono;

    if (state_ == 0) {
        HWND hwnd = Utils::appMainWindowHandle();

        if (hwnd) {
            Log::instance().out(L"Windscribe is running - sending close");
            // PostMessage WM_CLOSE will only work on an active window
            UINT dwActivateMessage = RegisterWindowMessage(wmActivateGui.c_str());
            PostMessage(hwnd, dwActivateMessage, 0, 0);
        }

        const auto start = chr::high_resolution_clock::now();
        auto curTime = chr::high_resolution_clock::now();
        while (hwnd)
        {
            curTime = chr::high_resolution_clock::now();
            if (curTime - start < kWindscribeClosingTimeout) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                Sleep(100);
                hwnd = Utils::appMainWindowHandle();
            }
            else {
                Log::instance().out(L"Timeout exceeded when trying to close Windscribe. Killing the process.");

                const wstring appName = Path::append(Utils::getSystemDir(), L"taskkill.exe");
                const wstring commandLine = L"/f /im " + ApplicationInfo::appExeName();
                const auto result = Utils::instExec(appName, commandLine, INFINITE, SW_HIDE);
                if (!result.has_value()) {
                    Log::instance().out(L"WARNING: an error was encountered attempting to start taskkill.exe.");
                    return -ERROR_OTHER;
                }
                else if (result.value() != NO_ERROR && result.value() != ERROR_WAIT_NO_CHILDREN) {
                    Log::instance().out(L"WARNING: unable to kill Windscribe (%lu).", result.value());
                    return -ERROR_OTHER;
                }
                else {
                    Log::instance().out(L"Windscribe was successfully killed.");
                    break;
                }
            }
        }
        Sleep(1000);
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
            if (!isFactoryReset_) {
                QSettings reg(QString::fromStdWString(ApplicationInfo::appRegistryKey()), QSettings::NativeFormat);
                reg.setValue("userId", "");
            }

            DWORD lastError = 0;
            if (!uninstallOldVersion(uninstallString, lastError)) {
                Log::instance().out(L"UninstallPrev::executeStep: uninstallOldVersion failed: %lu", lastError);
                if (lastError != 2) { // Any error other than "Not found"
                    return -ERROR_OTHER;
                }

                // Uninstall failed because the uninstaller doesn't exist.
                if (!isPrevInstall64Bit()) {
                    return -ERROR_UNINSTALL;
                }

                if (!extractUninstaller()) {
                    Log::instance().out(L"UninstallPrev::executeStep: could not extract uninstaller.");
                    return -ERROR_OTHER;
                }
                Log::instance().out(L"UninstallPrev::executeStep: successfully extracted uninstaller, trying again.");
                return 65;
            }
        }
        return 100;
    }
    return 100;
}

wstring UninstallPrev::getUninstallString()
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

bool UninstallPrev::isPrevInstall64Bit()
{
    QSettings reg(QString::fromStdWString(ApplicationInfo::uninstallerRegistryKey()), QSettings::NativeFormat);
    if (reg.contains(L"UninstallString")) {
        return true;
    }
    return false;
}

bool UninstallPrev::uninstallOldVersion(const wstring &uninstallString, DWORD &lastError)
{
    const wstring sUnInstallString = removeQuotes(uninstallString);
    DWORD error = 0;

    const auto res = Utils::instExec(sUnInstallString, L"/VERYSILENT", INFINITE, SW_HIDE, L"", &error);
    if (!res.has_value() || res.value() == MAXDWORD) {
        lastError = error;
        return false;
    }

    // uninstall process can return second phase processId, which we need wait to finish
    if (res.value() != 0) {
        wsl::Win32Handle processHandle(::OpenProcess(SYNCHRONIZE, FALSE, res.value()));
        if (processHandle.isValid()) {
            processHandle.wait(INFINITE);
        }
    }

    return true;
}

wstring UninstallPrev::removeQuotes(const wstring &str)
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

void UninstallPrev::doFactoryReset()
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

void UninstallPrev::stopService()
{
    try {
        wsl::ServiceControlManager scm;
        scm.stopService(ApplicationInfo::serviceName().c_str());
    }
    catch (system_error& ex) {
        Log::instance().out(L"UninstallPrev::stopService %hs (%lu)", ex.what(), ex.code().value());
    }
}

bool UninstallPrev::extractUninstaller()
{
    Log::instance().out(L"Extracting uninstaller from the archive");

    wsl::Archive archive;
    archive.setLogFunction([](const wstring &str) {
        Log::instance().out(str);
    });

    const wstring exePath = Utils::getExePath();
    if (exePath.empty()) {
        Log::instance().out(L"Could not get exe path");
        return false;
    }

    const wstring targetFolder = Path::extractDir(removeQuotes(getUninstallString()));

    if (!archive.extract(L"Windscribe", L"windscribe.7z", exePath, L"uninstall.exe", targetFolder)) {
        return false;
    }

    return true;
}
