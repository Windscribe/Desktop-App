#include "uninstallprev.h"

#include <QDeadlineTimer>
#include <QSettings>

#include <chrono>
#include <filesystem>
#include <shlobj_core.h>
#include <windows.h>
#include <spdlog/spdlog.h>

#include "installerenums.h"
#include "uninstall_info.h"
#include "../../../utils/applicationinfo.h"
#include "../../../utils/archive.h"
#include "../../../utils/path.h"
#include "../../../utils/utils.h"
#include "global_consts.h"
#include "servicecontrolmanager.h"
#include "../installer_utils.h"
#include "win32handle.h"

using namespace std;

/*
Enables or disables SeDebugPrivilege for this process, reporting through wasEnabled whether it was
already enabled so the caller can put the token back exactly as it found it.

Windows gives a service process a DACL that grants Administrators query access only - 0x1400,
PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION - so an elevated installer cannot
open the helper for PROCESS_TERMINATE on the DACL alone.  SeDebugPrivilege bypasses that check; it
is present but disabled in an elevated token, hence this call.  taskkill /f does the same thing.

Enabling grants this process nothing it could not already grant itself - any code running at high
integrity can enable a privilege its token already holds - but the enabled state is inherited by
child processes, and this block goes on to launch the previous install's uninstaller.  So the
caller holds it across the OpenProcess call and no longer.
*/
static bool setDebugPrivilege(bool enable, bool *wasEnabled = NULL)
{
    wsl::Win32Handle token;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, token.data())) {
        spdlog::warn("OpenProcessToken failed ({}).", ::GetLastError());
        return false;
    }

    TOKEN_PRIVILEGES privileges = {};
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);
    if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid)) {
        spdlog::warn("LookupPrivilegeValue for SeDebugPrivilege failed ({}).", ::GetLastError());
        return false;
    }

    TOKEN_PRIVILEGES previous = {};
    DWORD previousSize = sizeof(previous);
    const BOOL adjusted = ::AdjustTokenPrivileges(token.getHandle(), FALSE, &privileges, sizeof(privileges),
                                                  &previous, &previousSize);
    // AdjustTokenPrivileges reports success when it assigns nothing, so the last error has to be
    // checked as well.
    const DWORD lastError = ::GetLastError();
    if (!adjusted || lastError == ERROR_NOT_ALL_ASSIGNED) {
        spdlog::warn("Could not {} SeDebugPrivilege ({}).", (enable ? "enable" : "disable"), lastError);
        return false;
    }

    if (wasEnabled != NULL) {
        // PreviousState only carries the privileges the call actually modified, so an empty one
        // means the token already held SeDebugPrivilege in the state we asked for.  A LocalSystem
        // token - an Intune or SCCM deployment - has it enabled from the start and lands here;
        // reading that as "was disabled" would have the caller disable what it did not enable.
        *wasEnabled = (previous.PrivilegeCount == 0)
                          ? enable
                          : ((previous.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) != 0);
    }

    return true;
}

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

            // log the previous installed program version
            // Read the path of the installed client from the registry and determine Windscribe.exe version
            QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);
            if (reg.contains("applicationPath")) {
                const auto path = reg.value("applicationPath").toString().toStdWString() + L"\\Windscribe.exe";
                spdlog::info(L"Found a previous installation of the application: {}", InstallerUtils::getExecutableVersion(path));
            }

            if (!isFactoryReset_) {
                QSettings reg(QString::fromStdWString(ApplicationInfo::appRegistryKey()), QSettings::NativeFormat);
                reg.setValue("userId", "");
            }

            DWORD lastError = 0;
            if (!uninstallOldVersion(uninstallString, lastError)) {
                spdlog::error(L"UninstallPrev::executeStep: uninstallOldVersion failed: {}", lastError);
                if (lastError != 2) { // Any error other than "Not found"
                    return -wsl::ERROR_OTHER;
                }

                // Uninstall failed because the uninstaller doesn't exist.
                if (!isPrevInstall64Bit()) {
                    return -wsl::ERROR_UNINSTALL;
                }

                if (!extractUninstaller()) {
                    spdlog::error(L"UninstallPrev::executeStep: could not extract uninstaller.");
                    return -wsl::ERROR_OTHER;
                }
                spdlog::info(L"UninstallPrev::executeStep: successfully extracted uninstaller, trying again.");
                return 65;
            }
        } else {
            spdlog::info(L"UninstallPrev did not find a previous install and is skipping the uninstall step.");
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

    if (uninstallString.empty()) {
        // It's possible we arrived here because this is a virgin install.  In that case the installer reg key shouldn't exist,
        // nor will the Windscribe service be installed.
        wstring prevInstallPath;
        QSettings reg(QString::fromStdWString(ApplicationInfo::installerRegistryKey()), QSettings::NativeFormat);
        if (reg.contains("applicationPath")) {
            prevInstallPath = reg.value("applicationPath").toString().toStdWString();
            spdlog::warn(L"No uninstaller information found in the registry. Found previous installer application path in registry ({}).", prevInstallPath);
        } else {
            // As a last ditch effort, check if the Windscribe service is installed and use its path.
            try {
                wsl::ServiceControlManager scm;
                scm.openSCM(SC_MANAGER_CONNECT);
                const auto serviceName = ApplicationInfo::serviceName();
                if (scm.isServiceInstalled(serviceName.c_str())) {
                    scm.openService(serviceName.c_str(), SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
                    prevInstallPath = Path::extractDir(scm.exePath());
                    if (!prevInstallPath.empty()) {
                        spdlog::warn(L"No uninstaller information found in the registry. Found previous helper install in SCM ({}).", prevInstallPath);
                    }
                }
            }
            catch (std::system_error& ex) {
                spdlog::warn("getUninstallString failed to query helper install status: {}", ex.what());
            }
        }

        if (!prevInstallPath.empty()) {
            error_code ec;
            if (filesystem::exists(prevInstallPath, ec)) {
                uninstallString = Path::append(prevInstallPath, ApplicationInfo::uninstaller());
                // We need to write the missing uninstaller location info to the registry or the uninstaller will error out with
                // its "The uninstall cannot proceed. The installation directory does not match the removal directory." error.
                // Unfortunately cannot simply patch the uninstaller to fix this, as this might be an old uninstaller.
                UninstallInfo::addMissingUninstallInfo(uninstallString);
            } else {
                if (ec) {
                    spdlog::error("UninstallPrev::getUninstallString: filesystem::exists failed ({}).", ec.message());
                }
                spdlog::warn(L"Previous install path ({}) does not exist.", prevInstallPath);
            }
        }
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

/*
Stops the helper and does not return until it is stopped, the escalation below has run, or both have
failed.  The uninstaller we launch next deletes the service, and the Service block after it creates
the replacement; both fail (ERROR_SERVICE_MARKED_FOR_DELETE) against a helper that is still winding
down, which the user sees as HELPER_INSTALL.
*/
void UninstallPrev::stopService() const
{
    // The helper's stop is cooperative: it does not acknowledge the SCM until the command it is
    // executing returns, and some of those (netsh, WMI, ICS) have no bound of their own.  The wait
    // below is what a healthy but busy teardown needs; anything past it is a helper we cannot talk
    // our way out of, so we take the process down instead of proceeding against a live service.
    constexpr int kStopTimeoutMs = 20000;

    const wstring serviceName = ApplicationInfo::serviceName();

    try {
        wsl::ServiceControlManager scm;
        scm.openSCM(SC_MANAGER_CONNECT);

        if (!scm.isServiceInstalled(serviceName.c_str())) {
            spdlog::info(L"The {} service is not installed; nothing to stop.", serviceName);
            return;
        }

        // SERVICE_QUERY_CONFIG so the escalation can name the helper binary for its taskkill fallback.
        scm.openService(serviceName.c_str(), SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_STOP);

        error_code ec;
        const DWORD stateBefore = scm.queryServiceStatus(ec);
        spdlog::info("The helper is {} before the stop request.",
                     wsl::ServiceControlManager::serviceStatusToString(stateBefore));

        const auto started = std::chrono::steady_clock::now();
        const auto elapsedMs = [&started]() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - started).count();
        };

        // Succeeds for a helper that is already stopped, one we stop here, and one somebody else
        // is already stopping - the last of which used to return immediately without waiting.
        if (scm.stopService(ec, kStopTimeoutMs)) {
            spdlog::info("The helper is stopped ({} ms).", elapsedMs());
            return;
        }

        error_code stateEc;
        const DWORD stateAfter = scm.queryServiceStatus(stateEc);
        spdlog::error("The helper did not stop within {} ms - {} ({}).  It is now {}.",
                      elapsedMs(), ec.message(), ec.value(),
                      wsl::ServiceControlManager::serviceStatusToString(stateAfter));

        // The stop is cooperative, so the helper can acknowledge the SCM just after our wait gives
        // up.  A stopped helper is all the callers need; the escalation below has no process left
        // to work with and would report a live helper against one that is already gone.
        if (!stateEc && stateAfter == SERVICE_STOPPED) {
            spdlog::warn("The helper stopped on its own just after the wait expired ({} ms).", elapsedMs());
            return;
        }

        wsl::ServiceControlManager::logServiceStatusAndConfig(serviceName.c_str(), true,
                                                              [](const std::string &message) {
            spdlog::error("stopService: {}", message);
        });

        if (terminateService(scm)) {
            spdlog::warn("The helper had to be terminated to stop it ({} ms).", elapsedMs());
            return;
        }

        // The logging and the escalation above each take time the helper can use to finish its
        // stop, so the verdict below is only worth printing against what the SCM says now.
        const DWORD stateFinal = scm.queryServiceStatus(stateEc);
        if (!stateEc && stateFinal == SERVICE_STOPPED) {
            spdlog::warn("The helper stopped on its own while we were terminating it ({} ms).", elapsedMs());
            return;
        }

        // Nothing left to try here.  We still run the uninstall: the blocks that follow delete and
        // recreate the service and have waits of their own, so an install that can still succeed
        // should not be abandoned at this point.  The lines above name the reason if it does not.
        spdlog::error("Proceeding with the uninstall against a helper that is still running."
                      "  The service replacement may fail.");
    }
    catch (system_error& ex) {
        spdlog::error("UninstallPrev::stopService {} ({})", ex.what(), ex.code().value());
    }
}

/*
Last resort for a helper that will not acknowledge the SCM: kill the process hosting it.  The SCM
moves the service to SERVICE_STOPPED once the process is gone, which is all the uninstaller and the
Service block need.  Returns true only once the SCM confirms that.
*/
bool UninstallPrev::terminateService(wsl::ServiceControlManager &scm) const
{
    constexpr int kTerminateTimeoutMs = 5000;

    error_code ec;
    const DWORD processId = scm.queryServiceProcessId(ec);
    if (processId == 0) {
        if (ec) {
            spdlog::error("Could not identify the helper process to terminate - {} ({}).", ec.message(), ec.value());
        }
        else {
            // The SCM attributes no process to a service it considers stopped, so this is the same
            // late stop the caller checks for - not a failure, and not something to log as one.
            spdlog::warn("The SCM no longer attributes a process to the helper; nothing to terminate.");
        }
        return false;
    }

    // Named from the SCM's own record of the binary rather than assembled from the service name,
    // so both the identity check and the fallback still target the right image if the two ever
    // diverge.
    const wstring imagePath = removeQuotes(scm.exePath());
    const wstring exeName = Path::extractName(imagePath);
    spdlog::warn(L"Terminating the helper process {} ({}).", exeName, processId);

    // taskkill runs as its own elevated process and enables SeDebugPrivilege for itself, so it is
    // worth a try even when we could not do the same here.  It matches on the image name, so it
    // cannot hit an unrelated process the way a stale pid could.
    if (!killProcess(processId, imagePath, kTerminateTimeoutMs)) {
        spdlog::warn(L"Falling back to taskkill for {}.", exeName);
        if (exeName.empty() || taskKill(exeName) != NO_ERROR) {
            return false;
        }
    }

    // Process death and the SCM's bookkeeping are not simultaneous; the callers that follow read
    // the SCM, so that is what we have to see settle.
    if (!scm.waitForServiceStopped(ec, kTerminateTimeoutMs)) {
        spdlog::error("The helper process was terminated but the service is still not stopped - {} ({}).",
                      ec.message(), ec.value());
        return false;
    }

    return true;
}

/*
Terminates processId, but only after confirming through the handle that it really is the helper,
and waits up to waitMs for it to go away.  Returns false without logging an error for anything the
taskkill fallback in the caller can still recover from.
*/
bool UninstallPrev::killProcess(DWORD processId, const wstring &expectedImagePath, int waitMs) const
{
    bool wasEnabled = false;
    const bool adjusted = setDebugPrivilege(true, &wasEnabled);

    wsl::Win32Handle process(::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
                                           FALSE, processId));
    const DWORD openError = ::GetLastError();

    // Back to how we found it, before anything else runs and certainly before this block launches
    // the previous install's uninstaller, which would otherwise inherit the enabled privilege.
    if (adjusted && !wasEnabled) {
        setDebugPrivilege(false);
    }

    if (!process.isValid()) {
        spdlog::warn("Could not open the helper process {} ({}).", processId, openError);
        return false;
    }

    // The pid came from the SCM moments ago and the helper is in the middle of stopping, so it may
    // well have exited and had its pid reused since.  Terminating with SeDebugPrivilege in hand is
    // not something to do to a process we have not identified.  The handle pins whatever we opened,
    // so reading the image through it settles the identity for good.
    wchar_t imagePath[MAX_PATH] = {};
    DWORD imagePathLength = ARRAYSIZE(imagePath);
    if (!::QueryFullProcessImageName(process.getHandle(), 0, imagePath, &imagePathLength)) {
        spdlog::warn("Could not read the image path of process {} ({}).", processId, ::GetLastError());
        return false;
    }

    if (::_wcsicmp(imagePath, expectedImagePath.c_str()) != 0) {
        spdlog::warn(L"Process {} is {}, not the helper ({}); leaving it alone.",
                     processId, imagePath, expectedImagePath);
        return false;
    }

    if (!::TerminateProcess(process.getHandle(), 1)) {
        const DWORD lastError = ::GetLastError();
        // The helper may have finished its teardown on its own between the query above and here.
        // A process that is already gone is the outcome we wanted, whatever the call reported.
        if (process.wait(0) != WAIT_OBJECT_0) {
            spdlog::warn("Could not terminate the helper process {} ({}).", processId, lastError);
            return false;
        }
    }

    return (process.wait(waitMs) == WAIT_OBJECT_0);
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

    if (!archive.extract(L"Windscribe", L"windscribe.7z", exePath, ApplicationInfo::uninstaller(), targetFolder)) {
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

int UninstallPrev::taskKill(const std::wstring &exeName) const
{
    const wstring appName = Path::append(Utils::getSystemDir(), L"taskkill.exe");
    const wstring commandLine = L"/f /t /im " + exeName;
    const auto result = Utils::instExec(appName, commandLine, INFINITE, SW_HIDE);
    if (!result.has_value()) {
        spdlog::warn(L"WARNING: an error was encountered attempting to start taskkill.exe.");
        return -wsl::ERROR_KILL;
    }

    if (result.value() != NO_ERROR && result.value() != ERROR_WAIT_NO_CHILDREN) {
        spdlog::warn(L"WARNING: unable to kill {} ({}).", exeName, result.value());
        return -wsl::ERROR_KILL;
    }

    spdlog::info(L"taskkill executed on {} ({})", exeName, result.value());
    return NO_ERROR;
}
