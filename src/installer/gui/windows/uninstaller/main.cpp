#include <spdlog/spdlog.h>
#include <spdlog/sinks/callback_sink.h>

#include "arguments_parser.h"
#include "copy_and_run.h"
#include "registry.h"
#include "remove_directory.h"
#include "resource.h"
#include "uninstall.h"

#include "../utils/applicationinfo.h"
#include "../utils/path.h"
#include "../utils/persistent_log.h"
#include "../../../../client/client-common/utils/log/spdlog_utils.h"
#include "wsprocessmitigations.h"
#include "wsscopeguard.h"

// Let's compare where the uninstall file is located with where the program was originally installed
// If they are different then we do not uninstall, to avoid deleting 3-rd party files
static bool isRightInstallation(const std::wstring& uninstExeFile)
{
    bool isRightInstallation = false;

    HKEY hKey;
    const auto subkeyName = ApplicationInfo::uninstallerRegistryKey(false);
    const auto result = Registry::RegOpenKeyExView(HKEY_LOCAL_MACHINE, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey);
    if (result == ERROR_SUCCESS) {
        std::wstring path;
        if (Registry::RegQueryStringValue1(hKey, L"InstallLocation", path)) {
            isRightInstallation = Path::equivalent(Path::extractDir(uninstExeFile), path);
        } else {
            spdlog::warn(L"isRightInstallation failed to query the 'InstallLocation' value");
        }
        RegCloseKey(hKey);
    } else {
        spdlog::warn(L"isRightInstallation failed to open the uninstall registry key '{}' ({})", subkeyName, result);
    }
    return isRightInstallation;
}

// /SECONDPHASE="C:\Program Files\Windscribe\uninstall.exe" /VERYSILENT
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdParam);

    // Parse the arguments before initializing the logger: the uninstaller is two-phase
    // (it copies itself to a temp folder and relaunches), and the phase determines how
    // the persistent log file is opened below.
    ArgumentsParser argumentParser;
    const bool argsOk = argumentParser.parse();

    bool isSilent = argumentParser.getArg(L"/VERYSILENT", nullptr);
    std::wstring uninstExeFile;
    bool isSecondPhase = argumentParser.getArg(L"/SECONDPHASE", &uninstExeFile);

    // Incremental log in %ProgramData%\Windscribe: written entry-by-entry, so it survives
    // a crash and the removal of the install folder performed by this very process.
    // The first phase starts a new file (rotating the previous one to .1); the second
    // phase appends, so the whole uninstall session ends up in a single file.
    wsl::PersistentLog persistentLog;
    persistentLog.open(L"uninstaller.log",
                       isSecondPhase ? wsl::PersistentLog::OpenMode::Append : wsl::PersistentLog::OpenMode::Rotate);

    // Initialize logger: each entry goes to the persistent log file and to OutputDebugString.
    auto formatter = log_utils::createJsonFormatter();
    auto logCallback = [&persistentLog, &formatter](const spdlog::details::log_msg &msg) {
        spdlog::memory_buf_t dest;
        formatter->format(msg, dest);
        std::string str(dest.data(), dest.size());
        persistentLog.write(str);
        ::OutputDebugStringA(str.c_str());
    };

    auto logger = spdlog::callback_logger_mt("uninstall", logCallback);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);

    // The registry-held default logger funnels into logCallback, which references
    // persistentLog and formatter on this stack frame.  Release it on every exit path,
    // before those locals are destroyed: otherwise a stray spdlog call during the
    // static-destruction phase after WinMain returns would go through dangling
    // references.  The sink-less replacement makes any late spdlog call a harmless
    // no-op; replacing the default logger also drops the old one from the registry.
    auto loggingTeardownGuard = wsl::wsScopeGuard([&logger] {
        logger->flush();
        spdlog::set_default_logger(std::make_shared<spdlog::logger>("teardown"));
    });

    if (!isSecondPhase) {
        if (persistentLog.isOpen()) {
            spdlog::info(L"Persistent uninstaller log: {}", persistentLog.path());
        } else {
            spdlog::warn(L"Persistent uninstaller log unavailable; see debug output for details");
        }
        spdlog::info(L"Uninstalling Windscribe version {}", ApplicationInfo::version());
    }

    // Design Note:
    // Returning MAXDWORD to indicate failure state.  UninstallPrev::uninstallOldVersion() in the installer
    // expects the return value from this process to be a DWORD, representing zero (success), a valid process
    // ID, or a failure code.  It's not clear from the Windows docs if (DWORD)(-1) is possibly a valid
    // process ID.

    if (!argsOk) {
        spdlog::error(L"ArgumentsParser failed");
        return MAXDWORD;
    }
    if (!isSecondPhase) {
        uninstExeFile = argumentParser.getExecutablePath();
        spdlog::info(L"First phase started {}", uninstExeFile);
        if (!isRightInstallation(uninstExeFile)) {
            Uninstaller::messageBox(IDS_DIRECTORY_MISMATCH, MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
            return MAXDWORD;
        }

        return CopyAndRun::runFirstPhase(uninstExeFile, lpszCmdParam);
    }

    spdlog::info(L"Second phase started");
    Uninstaller::setSilent(isSilent);
    Uninstaller::setUninstExeFile(uninstExeFile, false);

    if (!isSilent) {
        int result = Uninstaller::messageBox(IDS_CONFIRM_UNINSTALL, MB_ICONQUESTION | MB_YESNO | MB_SETFOREGROUND);
        if (result == IDNO) {
            return MAXDWORD;
        }
    }

    const bool uninstallSucceeded = Uninstaller::RunSecondPhase();

    // On a successful interactive uninstall, remove the whole %ProgramData%\Windscribe
    // folder including the installer/uninstaller logs, so a full removal of the product
    // leaves no leftovers.  A silent uninstall is an upgrade driven by the installer --
    // the logs must survive it.  On failure the logs are kept: they are the diagnostics.
    // DelTree does not follow reparse points (it removes the link itself), so a
    // junction/symlink planted in the folder cannot redirect the deletion elsewhere.
    if (uninstallSucceeded && !isSilent) {
        const std::wstring logDir = wsl::PersistentLog::defaultDirectory();
        spdlog::info(L"Removing directory {}", logDir);
        // Close our own uninstaller.log handle; otherwise the deletion below would fail
        // with a sharing violation.  Subsequent log entries go to OutputDebugString only.
        persistentLog.close();
        if (!logDir.empty()) {
            // Retry a few times: the first-phase uninstaller may still briefly hold its
            // uninstaller.log handle.  The log is opened with FILE_SHARE_DELETE, but on
            // older Windows 10 builds the file name disappears only when the last handle
            // closes, which can make the directory removal fail as "not empty".
            for (int attempt = 0; attempt < 5; ++attempt) {
                if (RemoveDir::DelTree(logDir, true, true, true, false)) {
                    break;
                }
                ::Sleep(200);
            }
        }
    }

    spdlog::debug(L"WinMain finished");
    return 0;
}
