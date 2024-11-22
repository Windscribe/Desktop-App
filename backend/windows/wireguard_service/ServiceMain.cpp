#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "utils/servicecontrolmanager.h"
#include "utils/win32handle.h"
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
static HANDLE
getWindscribeClientProcessHandle()
{
    #ifdef USE_SIGNATURE_CHECK
    std::wstring clientExe = getProcessFolder() + L"\\Windscribe.exe";
    #else
    std::wstring clientExe(L"[any path]/Windscribe.exe");
    #endif

    wsl::Win32Handle hProcesses(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!hProcesses.isValid()) {
        throw std::system_error(::GetLastError(), std::generic_category(),
            "getWindscribeClientProcessHandle: CreateToolhelp32Snapshot(processes) failed");
    }

    PROCESSENTRY32 pe32;
    ::ZeroMemory(&pe32, sizeof(pe32));
    pe32.dwSize = sizeof(pe32);

    if (::Process32First(hProcesses.getHandle(), &pe32) == FALSE) {
        throw std::system_error(::GetLastError(), std::generic_category(),
            "getWindscribeClientProcessHandle: Process32First failed");
    }

    HANDLE hWindscribeClient = INVALID_HANDLE_VALUE;

    do
    {
        if (_wcsicmp(pe32.szExeFile, L"Windscribe.exe") == 0)
        {
            wsl::Win32Handle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pe32.th32ProcessID));

            if (!hProcess.isValid()) {
                throw std::system_error(::GetLastError(), std::generic_category(), "getWindscribeClientProcessHandle: OpenProcess failed");
            }

            // Only verify the exe path in signed builds.  This allows us to monitor builds
            // of the Windscribe client being run under the debugger.
            #ifdef USE_SIGNATURE_CHECK
            wchar_t szExePath[MAX_PATH];
            DWORD dwPathLen = ::GetModuleFileNameEx(hProcess.getHandle(), NULL, szExePath, MAX_PATH);
            if (dwPathLen == 0) {
                throw std::system_error(::GetLastError(), std::generic_category(), "getWindscribeClientProcessHandle: GetModuleFileNameEx failed");
            }

            if (_wcsicmp(clientExe.c_str(), szExePath) == 0)
            {
                hWindscribeClient = hProcess.release();
                break;
            }
            #else
            hWindscribeClient = hProcess.release();
            break;
            #endif
        }
    }
    while (::Process32Next(hProcesses.getHandle(), &pe32));

    if (hWindscribeClient == INVALID_HANDLE_VALUE) {
        throw std::system_error(ERROR_FILE_NOT_FOUND, std::generic_category(),
            "getWindscribeClientProcessHandle: could not locate the Windscribe client process");
    }

    spdlog::info(L"Windscribe WireGuard service monitoring {}", clientExe);

    return hWindscribeClient;
}

//---------------------------------------------------------------------------
static DWORD WINAPI
monitorClientStatus(LPVOID lpParam)
{
    try
    {
        wsl::Win32Handle hWindscribeClient(getWindscribeClientProcessHandle());

        DWORD dwWait = hWindscribeClient.wait(INFINITE, TRUE);
        if (dwWait == WAIT_OBJECT_0)
        {
            // We should only ever get here if the Windscribe client process has died unexpectedly.
            // During normal operation, the client app should ask the helper to shut this
            // process down gracefully when the app disconnects the tunnel.

            std::wstring serviceName;
            {
                // The wireguard tunnel DLL derives the service name from the config file name
                // we passed to WireGuardTunnelService() in main().
                std::filesystem::path path((const char*)lpParam);
                serviceName = L"WireGuardTunnel$" + path.stem().native();
            }

            wsl::ServiceControlManager svcCtrl;
            svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

            // This will instruct the WireGuardTunnelService in main() to exit, wait for it to do so,
            // then delete the service.
            svcCtrl.deleteService(serviceName.c_str(), true);
        }
    }
    catch (std::system_error& ex)
    {
        spdlog::error("Windscribe WireGuard service - {}", ex.what());
    }

    spdlog::info("Windscribe WireGuard service client monitor thread exiting");
    return NO_ERROR;
}

static VOID
stopMonitorThread(ULONG_PTR lpParam)
{
    // Do-nothing function that is queued to the thread's APC queue to get it to exit.
    // Cleaner, and faster, than having the thread wait on an event object that is
    // signalled when we want the thread to exit.
    (void)lpParam;
}


//---------------------------------------------------------------------------
int wmain(int argc, wchar_t *argv[])
{
    // Initialize logger
    std::wstring logPath = getProcessFolder() + + L"\\wireguard_service.log";

    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    try
    {
        if (log_utils::isOldLogFormat(logPath)) {
            std::filesystem::remove(logPath);
        }
        // Create rotation logger with 2 file with unlimited size
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto logger = spdlog::rotating_logger_mt("wg_service", logPath, SIZE_MAX, 1, true);

        // this will trigger flush on every log message
        logger->flush_on(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        printf("spdlog init failed: %s\n", ex.what());
        return 0;
    }

    spdlog::info("Windscribe WireGuard service starting");

    if (argc != 2)
    {
        spdlog::error("Windscribe WireGuard service - invalid command-line argument count: {}", argc);
        return 0;
    }

    std::wstring configFile(argv[1]);

    DWORD dwAttrib = ::GetFileAttributes(configFile.c_str());
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        spdlog::error(L"Windscribe WireGuard service - invalid config file path: {}", configFile);
        return 0;
    }

    std::wstring tunnelDLL = getProcessFolder() + L"\\tunnel.dll";

    HMODULE hTunnelDLL = ::LoadLibrary(tunnelDLL.c_str());
    if (hTunnelDLL == NULL)
    {
        spdlog::error(L"Windscribe WireGuard service - failed to load {} ({})", tunnelDLL, ::GetLastError());
        return 0;
    }

    typedef bool WireGuardTunnelService(const unsigned short* settings);

    WireGuardTunnelService* tunnelProc = (WireGuardTunnelService*)::GetProcAddress(hTunnelDLL, "WireGuardTunnelService");
    if (tunnelProc == NULL)
    {
        spdlog::error("Windscribe WireGuard service - failed to load WireGuardTunnelService entry point from tunnel.dll ({})", ::GetLastError());
        return 0;
    }

    spdlog::info("Starting WireGuard tunnel");

    // Disabling client app monitoring.  The tunnel should remain up if the client app crashes.
    // The client app will shut the tunnel down when it restarts.
    //WinUtils::Win32Handle hMonitorThread(::CreateThread(NULL, 0, monitorClientStatus,
    //                                                    (LPVOID)argv[1], 0, NULL));

    bool bResult = tunnelProc((const unsigned short*)configFile.c_str());

    if (!bResult) {
        spdlog::error("Windscribe WireGuard service - WireGuardTunnelService from tunnel.dll failed");
    }

    //if (hMonitorThread.isValid() && (hMonitorThread.wait(0) != WAIT_OBJECT_0))
    //{
    //    ::QueueUserAPC(stopMonitorThread, hMonitorThread.getHandle(), 0);
    //    hMonitorThread.wait(5000);
    //}

    spdlog::info("Windscribe WireGuard service stopped");

    ::FreeLibrary(hTunnelDLL);

    return 0;
}
