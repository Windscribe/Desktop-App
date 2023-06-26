#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>

#include <chrono>
#include <codecvt>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <sstream>

#include <boost/filesystem/path.hpp>

#include "utils/servicecontrolmanager.h"
#include "utils/win32handle.h"


// C:\Code\ThirdParty\Wireguard\wireguard-windows-0.5.3\docs\enterprise.md has useful info
// on how this service works.

// If the WireGuardTunnelService proc from tunnel.dll is failing during startup, nothing may
// be displayed in the wireguard ring log to help diagnose the issue.  Run this binary as a
// regular command-line app in that case, as tunnel.dll dumps to stderr during startup.

static std::chrono::system_clock::time_point PROCESS_START_TIME;


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

    boost::filesystem::path path(buffer);
    return path.parent_path().native();
}

//---------------------------------------------------------------------------
static std::wstring
logFilename(bool bPrevious = false)
{
    std::wostringstream stream;
    stream << getProcessFolder() << (bPrevious ? L"\\WireguardServiceLog_prev.txt" : L"\\WireguardServiceLog.txt");
    return stream.str();
}

//---------------------------------------------------------------------------
static void
resetLogFile()
{
    std::wstring logFile = logFilename();
    DWORD dwAttrib = ::GetFileAttributes(logFile.c_str());
    if (dwAttrib != INVALID_FILE_ATTRIBUTES)
    {
        std::wstring prevLogFile = logFilename(true);
        ::CopyFile(logFile.c_str(), prevLogFile.c_str(), FALSE);
        ::DeleteFile(logFile.c_str());
    }
}

//---------------------------------------------------------------------------
static void
debugOut(const char *str, ...)
{
    std::ofstream ofs;
    ofs.open(logFilename(), std::ofstream::out | std::ofstream::app);

    if (ofs.is_open())
    {
        auto timepoint = std::chrono::system_clock::now();
        auto coarse    = std::chrono::system_clock::to_time_t(timepoint);

        struct tm tmNow;
        if (::gmtime_s(&tmNow, &coarse) == 0)
        {
            auto msNow   = std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint);
            auto msStart = std::chrono::time_point_cast<std::chrono::milliseconds>(PROCESS_START_TIME);

            ofs << std::put_time(&tmNow, "[%d%m%y %H:%M:%S:")
                << std::setw(3) << msNow.time_since_epoch().count() % 1000 << ' '
                << std::setw(10) << std::setprecision(3) << std::setfill(' ')
                << (msNow.time_since_epoch().count() - msStart.time_since_epoch().count()) / 1000.0
                << "] [wg_service]\t ";
        }

        va_list args;
        va_start (args, str);
        char buf[1024];
        vsnprintf_s(buf, sizeof(buf), _TRUNCATE, str, args);
        va_end (args);

        ofs << buf << std::endl;
    }
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

    debugOut("Windscribe WireGuard service monitoring %ls", clientExe.c_str());

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
                boost::filesystem::path path((const char*)lpParam);
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
        debugOut("Windscribe WireGuard service - %s", ex.what());
    }

    debugOut("Windscribe WireGuard service client monitor thread exiting");
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
    PROCESS_START_TIME = std::chrono::system_clock::now();

    resetLogFile();

    debugOut("Windscribe WireGuard service starting");

    if (argc != 2)
    {
        debugOut("Windscribe WireGuard service - invalid command-line argument count: %d", argc);
        return 0;
    }

    std::wstring configFile(argv[1]);

    DWORD dwAttrib = ::GetFileAttributes(configFile.c_str());
    if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        debugOut("Windscribe WireGuard service - invalid config file path: %ls", configFile.c_str());
        return 0;
    }

    std::wstring tunnelDLL = getProcessFolder() + L"\\tunnel.dll";

    HMODULE hTunnelDLL = ::LoadLibrary(tunnelDLL.c_str());
    if (hTunnelDLL == NULL)
    {
        debugOut("Windscribe WireGuard service - failed to load %ls (%d)", tunnelDLL.c_str(), ::GetLastError());
        return 0;
    }

    typedef bool WireGuardTunnelService(const unsigned short* settings);

    WireGuardTunnelService* tunnelProc = (WireGuardTunnelService*)::GetProcAddress(hTunnelDLL, "WireGuardTunnelService");
    if (tunnelProc == NULL)
    {
        debugOut("Windscribe WireGuard service - failed to load WireGuardTunnelService entry point from tunnel.dll (%d)", ::GetLastError());
        return 0;
    }

    debugOut("Starting WireGuard tunnel");

    // Disabling client app monitoring.  The tunnel should remain up if the client app crashes.
    // The client app will shut the tunnel down when it restarts.
    //WinUtils::Win32Handle hMonitorThread(::CreateThread(NULL, 0, monitorClientStatus,
    //                                                    (LPVOID)argv[1], 0, NULL));

    bool bResult = tunnelProc((const unsigned short*)configFile.c_str());

    if (!bResult) {
        debugOut("Windscribe WireGuard service - WireGuardTunnelService from tunnel.dll failed");
    }

    //if (hMonitorThread.isValid() && (hMonitorThread.wait(0) != WAIT_OBJECT_0))
    //{
    //    ::QueueUserAPC(stopMonitorThread, hMonitorThread.getHandle(), 0);
    //    hMonitorThread.wait(5000);
    //}

    // Delete the config file.
    dwAttrib = ::GetFileAttributes(configFile.c_str());
    if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
        ::DeleteFile(configFile.c_str());
    }

    debugOut("Windscribe WireGuard service stopped");

    ::FreeLibrary(hTunnelDLL);

    return 0;
}
