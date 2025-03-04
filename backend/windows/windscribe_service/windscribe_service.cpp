#include "all_headers.h"

#include <conio.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "../../../client/common/utils/log/spdlog_utils.h"
#include "../../../client/common/utils/crashhandler.h"
#include "changeics/icsmanager.h"
#include "dns_firewall.h"
#include "executecmd.h"
#include "firewallfilter.h"
#include "fwpm_wrapper.h"
#include "ioutils.h"
#include "ipc/servicecommunication.h"
#include "ipv6_firewall.h"
#include "openvpncontroller.h"
#include "process_command.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/win32handle.h"

#define SERVICE_NAME  (L"WindscribeService")
#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_hThread = NULL;
std::atomic<bool>     g_ServiceExiting = false;

VOID WINAPI serviceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI serviceCtrlHandler(DWORD);
DWORD WINAPI serviceWorkerThread(LPVOID lpParam);
BOOL isElevated();
void stopServiceThreadApc(ULONG_PTR lpParam);

int main(int argc, char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "-firewall_off") == 0) {
            if (isElevated()) {
                FwpmWrapper fwpmWrapper;
                if (fwpmWrapper.initialize()) {
                    Ipv6Firewall::instance(&fwpmWrapper).enableIPv6();
                    DnsFirewall::instance(&fwpmWrapper).disable();
                    FirewallFilter::instance(&fwpmWrapper).off();
                    SplitTunneling::removeAllFilters(fwpmWrapper);
                    printf("Windscribe firewall deleted.\n");
                } else {
                    printf("Failed to initialize access to the Windows firewall manager.\n");
                }
            } else {
                printf("Please run the program with administrator rights.\n");
            }
            return 0;
        }
    }

    // Initialize logger
    std::wstring logPath = Utils::getExePath() + L"\\windscribe_service.log";
    auto formatter = log_utils::createJsonFormatter();
    spdlog::set_formatter(std::move(formatter));

    try
    {
        if (log_utils::isOldLogFormat(logPath)) {
            // Use the nothrow version of remove since the removal is not critical and we would
            // like to continue with spdlog creation if the removal does fail.
            std::error_code ec;
            std::filesystem::remove(logPath, ec);
        }
        // Create rotation logger with 2 file with unlimited size
        // rotate it on open, the first file is the current log, the 2nd is the previous log
        auto logger = spdlog::rotating_logger_mt("service", logPath, SIZE_MAX, 1, true);

        // this will trigger flush on every log message
        logger->flush_on(spdlog::level::trace);
        spdlog::set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        // Allow user/us to see this error when the program is running as a service.
        Utils::debugOut("spdlog init failed: %s", ex.what());
        return 0;
    }

#if defined(ENABLE_CRASH_REPORTS)
    Debug::CrashHandler::setModuleName(L"service");
    Debug::CrashHandler::instance().bindToProcess();
#endif

#ifdef _DEBUG
    // for debug (run without service)
    HANDLE hThread = CreateThread(NULL, 0, serviceWorkerThread, NULL, 0, NULL);
    _getch();
    g_ServiceExiting = true;
    QueueUserAPC(stopServiceThreadApc, hThread, 0);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
#else

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)serviceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcher(serviceTable) == FALSE) {
        spdlog::error("StartServiceCtrlDispatcher failed {}", ::GetLastError());
        return 0;
    }
#endif

    return 0;
}

static BOOL isElevated()
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

static void stopServiceThreadApc(ULONG_PTR lpParam)
{
    // No-op function queued to the service worker thread's APC queue to signal it to exit.
    (void)lpParam;
}

static void stopServiceThread()
{
    g_ServiceExiting = true;
    if (g_hThread != NULL) {
        ::QueueUserAPC(stopServiceThreadApc, g_hThread, 0);
    }
}

VOID WINAPI serviceMain(DWORD, LPTSTR *)
{
    // Register our service control handler with the SCM
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, serviceCtrlHandler);

    if (g_StatusHandle == NULL) {
        spdlog::error("RegisterServiceCtrlHandler failed {}", ::GetLastError());
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        spdlog::error("SetServiceStatus(start pending) failed {}", ::GetLastError());
    }

    g_ServiceExiting = false;

    // Start a thread that will perform the main task of the service
    g_hThread = ::CreateThread(NULL, 0, serviceWorkerThread, NULL, 0, NULL);
    if (g_hThread == NULL) {
        spdlog::error("serviceMain CreateThread failed {}", ::GetLastError());
    }
    else {
        // Wait until our worker thread exits signaling that the service needs to stop
        WaitForSingleObject(g_hThread, INFINITE);
    }

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        spdlog::error("SetServiceStatus(stopped) failed {}", ::GetLastError());
    }

    if (g_hThread != NULL) {
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
}

VOID WINAPI serviceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:

        spdlog::info("SERVICE_CONTROL_STOP message received");

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            break;
        }

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
            spdlog::error("SetServiceStatus(stop pending) failed {}", ::GetLastError());
        }

        stopServiceThread();
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        spdlog::info("SERVICE_CONTROL_SHUTDOWN message received");
        stopServiceThread();
        if (g_hThread != NULL) {
            WaitForSingleObject(g_hThread, INFINITE);
            CloseHandle(g_hThread);
            g_hThread = NULL;
        }
        break;
    default:
        break;
    }
}

static BOOL CreateDACL(SECURITY_ATTRIBUTES *pSA)
{
    const TCHAR * szSD = TEXT("D:")   // Discretionary ACL
        TEXT("(D;OICI;GA;;;AN)")      // Deny access to anonymous logon
        TEXT("(A;OICI;GRGWGX;;;BG)")  // Allow access to built-in guests
        TEXT("(A;OICI;GRGWGX;;;AU)")  // Allow read/write/execute to authenticated users
        TEXT("(A;OICI;GA;;;BA)");     // Allow full control to administrators

    if (NULL == pSA) {
        return FALSE;
    }

    return ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &(pSA->lpSecurityDescriptor), NULL);
}

static HANDLE CreatePipe()
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    if (!CreateDACL(&sa)) {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE hPipe = ::CreateNamedPipe(SERVICE_PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                     PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                     1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, &sa);

    if (hPipe == INVALID_HANDLE_VALUE) {
        spdlog::error("CreateNamedPipe failed {}", ::GetLastError());
    }

    return hPipe;
}

static bool writeMessagePacketResult(HANDLE hPipe, HANDLE hIOEvent, MessagePacketResult &mpr)
{
    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << mpr;
    const std::string str = stream.str();

    // first 4 bytes - size of buffer
    const auto sizeOfBuf = static_cast<unsigned long>(str.size());
    if (IOUtils::writeAll(hPipe, hIOEvent, (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
        if (sizeOfBuf > 0) {
            bool bRet = IOUtils::writeAll(hPipe, hIOEvent, str.c_str(), sizeOfBuf);
            return bRet;
        }
    }

    return false;
}

static void processClientRequests(HANDLE hPipe)
{
    if (!Utils::verifyWindscribeProcessPath(hPipe)) {
        return;
    }

    wsl::Win32Handle ioCompletedEvent(::CreateEvent(NULL, TRUE, TRUE, NULL));
    if (!ioCompletedEvent.isValid()) {
        spdlog::error("CreateEvent for client IPC IO completion failed {}", ::GetLastError());
        return;
    }

    while (!g_ServiceExiting) {
        int cmdId;
        if (!IOUtils::readAll(hPipe, ioCompletedEvent.getHandle(), (char *)&cmdId, sizeof(cmdId))) {
            break;
        }

        unsigned long sizeOfBuf;
        if (!IOUtils::readAll(hPipe, ioCompletedEvent.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
            break;
        }

        std::string strData;
        if (sizeOfBuf > 0) {
            std::vector<char> buffer(sizeOfBuf);
            if (!IOUtils::readAll(hPipe, ioCompletedEvent.getHandle(), buffer.data(), sizeOfBuf)) {
                break;
            }
            strData = std::string(buffer.begin(), buffer.end());
        }

        MessagePacketResult mpr = processCommand(cmdId, strData);
        writeMessagePacketResult(hPipe, ioCompletedEvent.getHandle(), mpr);
        ::FlushFileBuffers(hPipe);
    }
}

DWORD WINAPI serviceWorkerThread(LPVOID)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    BIND_CRASH_HANDLER_FOR_THREAD();

    FwpmWrapper fwpmWrapper;
    if (!fwpmWrapper.initialize()) {
        return 0;
    }

    spdlog::info("Service started");

    HANDLE hPipe = CreatePipe();
    if (hPipe == INVALID_HANDLE_VALUE) {
        return 0;
    }

    HANDLE hClientConnectedEvent = ::CreateEvent(NULL, TRUE, TRUE, NULL);
    if (hClientConnectedEvent == NULL) {
        spdlog::error("CreateEvent for client connection failed {}", ::GetLastError());
        return 0;
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE) {
        spdlog::error("SetServiceStatus(running) failed {}", ::GetLastError());
    }

    // Initialize singletons
    DnsFirewall::instance(&fwpmWrapper);
    FirewallFilter::instance(&fwpmWrapper);
    Ipv6Firewall::instance(&fwpmWrapper);
    SplitTunneling::instance(&fwpmWrapper);

    OVERLAPPED overlapped;

    while (!g_ServiceExiting) {
        ::ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = hClientConnectedEvent;

        BOOL result = ::ConnectNamedPipe(hPipe, &overlapped);
        if (result == FALSE) {
            DWORD lastError = ::GetLastError();
            if ((lastError != ERROR_IO_PENDING) && (lastError != ERROR_PIPE_CONNECTED)) {
                spdlog::error("ConnectNamedPipe failed {}", lastError);
                break;
            }
        }

        DWORD dwWait = ::WaitForSingleObjectEx(hClientConnectedEvent, INFINITE, TRUE);
        if (dwWait == WAIT_OBJECT_0) {
            processClientRequests(hPipe);
            ::DisconnectNamedPipe(hPipe);
        } else {
            if (dwWait == WAIT_FAILED) {
                spdlog::error("WaitForSingleObjectEx(client connect event) failed {}", ::GetLastError());
            }
            break;
        }
    }

    CloseHandle(hClientConnectedEvent);
    CloseHandle(hPipe);

    // turn off split tunneling
    CMD_CONNECT_STATUS connectStatus = { 0 };
    connectStatus.isConnected = false;
    SplitTunneling::instance().setConnectStatus(connectStatus);

    // Since we cannot control the order of destruction of these singletons, ensure they do not attempt
    // to reference the FwpmWrapper after it is released.
    DnsFirewall::instance().release();
    FirewallFilter::instance().release();
    Ipv6Firewall::instance().release();
    SplitTunneling::instance().release();

    // Uses COM, so must release it before we CoUninitialize() below.
    IcsManager::instance().release();

    ActiveProcesses::instance().release();
    ExecuteCmd::instance().release();
    OpenVPNController::instance().release();

    CoUninitialize();
    spdlog::info("Service stopped");

    return ERROR_SUCCESS;
}
