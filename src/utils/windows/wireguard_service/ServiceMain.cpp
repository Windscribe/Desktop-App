#include "ws_branding.h"
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "utils/log/spdlog_utils.h"

// C:\Code\ThirdParty\Wireguard\wireguard-windows-0.5.3\docs\enterprise.md has useful info
// on how this service works.

// If the WireGuardTunnelService proc from tunnel.dll is failing during startup, nothing may
// be displayed in the wireguard ring log to help diagnose the issue.  Run this binary as a
// regular command-line app in that case, as tunnel.dll dumps to stderr during startup.

//---------------------------------------------------------------------------
static std::wstring
getProcessFolder()
{
    wchar_t buffer[MAX_PATH];
    DWORD dwPathLen = ::GetModuleFileName(NULL, buffer, MAX_PATH);
    if (dwPathLen == 0) {
        throw std::system_error(::GetLastError(), std::generic_category(),
                                "GetProcessFolder: GetModuleFileName failed");
    }
    std::filesystem::path path(buffer);
    return path.parent_path().native();
}

//---------------------------------------------------------------------------
int wmain(int argc, wchar_t *argv[])
{
    // Initialize logger
    std::wstring logPath = getProcessFolder() + + L"\\wireguard_service.log";

    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    try {
        if (log_utils::isOldLogFormat(logPath)) {
            std::filesystem::remove(logPath);
        }
        // Create rotation logger with 2 file with max size 2MB each
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto logger = spdlog::rotating_logger_mt("wg_service", logPath, 2 * 1024 * 1024, 1, true);

        // this will trigger flush on every log message
        logger->flush_on(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex) {
        printf("spdlog init failed: %s\n", ex.what());
        return 0;
    }

    spdlog::info(WS_PRODUCT_NAME " WireGuard service starting");

    if (argc < 2 || argc > 3) {
        spdlog::error("Invalid command-line argument count: {}", argc);
        return 0;
    }

    std::wstring configFile(argv[1]);

    DWORD dwAttrib = ::GetFileAttributes(configFile.c_str());
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        spdlog::error(L"Invalid config file path: {}", configFile);
        return 0;
    }

    const bool useAmneziaWG = (argc == 3 && (wcscmp(argv[2], L"--amneziawg") == 0));

    std::wstring tunnelDLL = getProcessFolder();
    if (useAmneziaWG) {
        tunnelDLL += L"\\amneziawgtunnel.dll";
    } else {
        tunnelDLL += L"\\tunnel.dll";
    }

    HMODULE hTunnelDLL = ::LoadLibrary(tunnelDLL.c_str());
    if (hTunnelDLL == NULL) {
        spdlog::error(L"Failed to load {} ({})", tunnelDLL, ::GetLastError());
        return 0;
    }

    typedef bool WireGuardTunnelService(const unsigned short* settings);
    typedef BOOL AmneziaWGTunnelService(LPCWSTR settings, LPCWSTR adapterName);

    bool startResult;
    if (useAmneziaWG) {
        AmneziaWGTunnelService* tunnelProc = (AmneziaWGTunnelService*)::GetProcAddress(hTunnelDLL, "WireGuardTunnelService");
        if (tunnelProc == NULL) {
            spdlog::error("Failed to load WireGuardTunnelService entry point from amneziawgtunnel.dll ({})", ::GetLastError());
            return 0;
        }

        // AmneziaWG expects us to pass the contents of the config file as the first parameter of WireGuardTunnelService.
        std::wifstream configStream(configFile);
        if (!configStream.is_open()) {
            spdlog::error(L"Failed to open config file: {}", configFile);
            return 0;
        }

        std::wstring configContents((std::istreambuf_iterator<wchar_t>(configStream)),
                                    std::istreambuf_iterator<wchar_t>());
        configStream.close();

        // Need to use the same adapter name as regular WireGuard as we look for this name in the client app.
        startResult = tunnelProc(configContents.c_str(), WS_APP_IDENTIFIER_W L"Wireguard");
    } else {
        WireGuardTunnelService* tunnelProc = (WireGuardTunnelService*)::GetProcAddress(hTunnelDLL, "WireGuardTunnelService");
        if (tunnelProc == NULL) {
            spdlog::error("Failed to load WireGuardTunnelService entry point from tunnel.dll ({})", ::GetLastError());
            return 0;
        }

        startResult = tunnelProc((const unsigned short*)configFile.c_str());
    }

    if (!startResult) {
        spdlog::error("WireGuardTunnelService failed to start the tunnel");
    } else {
        spdlog::info("{} tunnel started", useAmneziaWG ? "AmneziaWG" : "WireGuard");
    }

    spdlog::info(WS_PRODUCT_NAME " WireGuard service stopped");

    ::FreeLibrary(hTunnelDLL);

    return 0;
}
