#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

#include "arguments_parser.h"
#include "copy_and_run.h"
#include "registry.h"
#include "resource.h"
#include "uninstall.h"

#include "../utils/applicationinfo.h"
#include "../utils/path.h"
#include "../../../client/common/utils/log/spdlog_utils.h"

// Set the DLL load directory to the system directory before entering WinMain().
struct LoadSystemDLLsFromSystem32
{
    LoadSystemDLLsFromSystem32() {
        // Remove the current directory from the search path for dynamically loaded
        // DLLs as a precaution.  This call has no effect for delay load DLLs.
        SetDllDirectory(L"");
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
} loadSystemDLLs;


// Let's compare where the uninstall file is located with where the program was originally installed
// If they are different then we do not uninstall, to avoid deleting 3-rd party files
static bool isRightInstallation(const std::wstring& uninstExeFile)
{
    std::wstring subkeyName = ApplicationInfo::uninstallerRegistryKey(false);

    bool bRet = false;
    HKEY hKey;
    if (Registry::RegOpenKeyExView(HKEY_LOCAL_MACHINE, subkeyName.c_str(), 0, KEY_QUERY_VALUE, hKey) == ERROR_SUCCESS) {
        std::wstring path;
        if (Registry::RegQueryStringValue1(hKey, L"InstallLocation", path)) {
            bRet = Path::equivalent(Path::extractDir(uninstExeFile), path);
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

// /SECONDPHASE="C:\Program Files\Windscribe\uninstall.exe" /VERYSILENT
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdParam);

    // Initialize logger (logging using OutputDebugString)
    auto logger = spdlog::create<spdlog::sinks::msvc_sink_mt>("uninstall", false);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);

    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    // Design Note:
    // Returning MAXDWORD to indicate failure state.  UninstallPrev::uninstallOldVersion() in the installer
    // expects the return value from this process to be a DWORD, representing zero (success), a valid process
    // ID, or a failure code.  It's not clear from the Windows docs if (DWORD)(-1) is possibly a valid
    // process ID.

    ArgumentsParser argumentParser;
    if (!argumentParser.parse()) {
        spdlog::error(L"ArgumentsParser failed");
        return MAXDWORD;
    }

    bool isSilent = argumentParser.getArg(L"/VERYSILENT", nullptr);
    std::wstring uninstExeFile;
    bool isSecondPhase = argumentParser.getArg(L"/SECONDPHASE", &uninstExeFile);
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

    Uninstaller::RunSecondPhase();

    spdlog::debug(L"WinMain finished");
    return 0;
}
