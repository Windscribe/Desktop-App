#include "ws_branding.h"
#include "helper.h"

#include <sddl.h>

#include <spdlog/spdlog.h>

#include "active_processes.h"
#include "changeics/icsmanager.h"
#include "dns_firewall.h"
#include "firewallfilter.h"
#include "firewallonboot.h"
#include "ipv6_firewall.h"
#include "openvpncontroller.h"
#include "process_command.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/crashhandler.h"
#include "utils/wsscopeguard.h"

namespace wsl
{

static void stopClientHandlerThreadApc(ULONG_PTR lpParam)
{
    // No-op function queued to the client handler thread's APC queue to signal it to exit.
    // This will break the client handler thread out of its wait for a client pipe connection
    // or client data to arrive on the pipe.
    (void)lpParam;
}

Helper::Helper() : Win32Service(WS_APP_IDENTIFIER_W L"Service")
{
}

Helper::~Helper()
{
}

bool Helper::onInit()
{
    spdlog::info("Helper initializing");

    if (!fwpmWrapper_.initialize()) {
        return false;
    }

    setStatus(SERVICE_START_PENDING);

    if (!createClientPipe()) {
        return false;
    }

    setStatus(SERVICE_START_PENDING);

    // Initialize singletons
    DnsFirewall::instance(&fwpmWrapper_);
    FirewallFilter::instance(&fwpmWrapper_);
    Ipv6Firewall::instance(&fwpmWrapper_);
    SplitTunneling::instance(&fwpmWrapper_);

    setStatus(SERVICE_START_PENDING);

    // Check if firewall on boot is enabled
    if (!FirewallOnBootManager::instance().isEnabled()) {
        spdlog::info("Firewall on boot is not enabled, turning off firewall");
        FirewallFilter::instance().off();
    }

    return true;
}

void Helper::runService()
{
    clientHandlerThread_.setHandle(::CreateThread(NULL, 0, clientHandlerThreadProc, this, 0, NULL));
    if (!clientHandlerThread_.isValid()) {
        spdlog::error("Failed to create the client handler thread: {}", ::GetLastError());
        return;
    }

    spdlog::info("Helper running");

    clientHandlerThread_.wait(INFINITE);
    clientHandlerThread_.closeHandle();
    clientPipe_.closeHandle();

    setStatus(SERVICE_STOP_PENDING, 1000);

    // Turn off split tunneling
    ConnectStatus connectStatus = { 0 };
    connectStatus.isConnected = false;
    SplitTunneling::instance().setConnectStatus(connectStatus);

    setStatus(SERVICE_STOP_PENDING, 1000);

    // Since we cannot control the order of destruction of these singletons, ensure they do not attempt
    // to reference the FwpmWrapper after it is released.
    DnsFirewall::instance().release();
    FirewallFilter::instance().release();

    setStatus(SERVICE_STOP_PENDING, 1000);
    Ipv6Firewall::instance().release();
    SplitTunneling::instance().release();

    ActiveProcesses::instance().release();
    ExecuteCmd::instance().release();

    setStatus(SERVICE_STOP_PENDING, 1000);
    OpenVPNController::instance().release();

    fwpmWrapper_.release();

    spdlog::info("Helper stopped");
}

void Helper::onStopRequest()
{
    stop();
}

void Helper::onWindowsShutdown()
{
    spdlog::info("Windows shutdown notification received");
    stop();
}

void Helper::onUserControl(DWORD controlCode)
{
    (void)controlCode;
}

void Helper::stop()
{
    // Inform the SCM that we are processing the stop/shutdown request.
    setStatus(SERVICE_STOP_PENDING, 1000);

    if (clientPipe_.isValid()) {
        ::CancelIoEx(clientPipe_.getHandle(), NULL);
    }

    if (clientHandlerThread_.isValid()) {
        ::QueueUserAPC(stopClientHandlerThreadApc, clientHandlerThread_.getHandle(), 0);
    }
}

bool Helper::createClientPipe()
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    // Allow only interactive-logon tokens (S-1-5-4, "IU"). Services, network logons, batch jobs,
    // and anonymous all lack this SID, so they cannot open the pipe even if they know the name.
    // This is the DACL-layer counterpart to the Session 0 reject in Utils::verifyAppProcessPath.
    const TCHAR * szSD = TEXT("D:")
        TEXT("(D;OICI;GA;;;AN)")       // Deny anonymous logon
        TEXT("(A;OICI;GRGWGX;;;IU)");  // Allow read/write/execute to interactive users

    if (!::ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
        return false;
    }
    auto freeSD = wsScopeGuard([&] {
        ::LocalFree(sa.lpSecurityDescriptor);
    });

    clientPipe_.setHandle(::CreateNamedPipe(L"\\\\.\\pipe\\" WS_APP_IDENTIFIER_W L"Service", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                            1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, &sa));
    if (!clientPipe_.isValid()) {
        spdlog::error("Failed to create the client commands pipe: {}", ::GetLastError());
        return false;
    }

    return true;
}

void Helper::processClientRequests()
{
#if !defined(WINDSCRIBE_DEV_MODE)
    if (!Utils::verifyAppProcessPath(clientPipe_.getHandle())) {
        return;
    }
#endif

    Win32Handle ioCompletedEvent(::CreateEvent(NULL, TRUE, TRUE, NULL));
    if (!ioCompletedEvent.isValid()) {
        spdlog::error("CreateEvent for client IPC IO completion failed: {}", ::GetLastError());
        return;
    }

    while (!serviceExiting()) {
        if (!readAll(ioCompletedEvent.getHandle(), (char *)&clientMessage_.cmdId, sizeof(clientMessage_.cmdId))) {
            break;
        }

        if (!readAll(ioCompletedEvent.getHandle(), (char *)&clientMessage_.requestSize, sizeof(clientMessage_.requestSize))) {
            break;
        }

        // Reject oversized frames before any size-derived allocation. Disconnecting
        // the client (break out of the request loop) is the safe response to any
        // local caller — authorized or otherwise — that produces a malformed frame.
        if (clientMessage_.requestSize > kMaxHelperFrameSize) {
            spdlog::error("Helper IPC: rejecting frame with invalid length {}", clientMessage_.requestSize);
            break;
        }

        clientMessage_.request.clear();
        if (clientMessage_.requestSize > 0) {
            try {
                clientMessage_.request.resize(clientMessage_.requestSize);
                if (!readAll(ioCompletedEvent.getHandle(), clientMessage_.request.data(), clientMessage_.requestSize)) {
                    break;
                }
            } catch (const std::bad_alloc &) {
                spdlog::error("Helper IPC: bad_alloc for frame size {}", clientMessage_.requestSize);
                break;
            }
        }

        clientMessage_.response.clear();
        try {
            clientMessage_.response = processCommand((HelperCommand)clientMessage_.cmdId, clientMessage_.request);
        } catch (const std::exception &ex) {
            spdlog::error("Helper IPC: processCommand({}) exception: {}", clientMessage_.cmdId, ex.what());
        }

        clientMessage_.responseSize = static_cast<unsigned long>(clientMessage_.response.size());
        if (writeAll(ioCompletedEvent.getHandle(), (char *)&clientMessage_.responseSize, sizeof(clientMessage_.responseSize))) {
            if (clientMessage_.responseSize > 0) {
                writeAll(ioCompletedEvent.getHandle(), clientMessage_.response.c_str(), clientMessage_.responseSize);
            }
        }
    }
}

DWORD WINAPI Helper::clientHandlerThreadProc(LPVOID lpParameter)
{
    Helper* pThis = static_cast<Helper*>(lpParameter);

    // Required for the IcsManager class.
    ::CoInitializeEx(0, COINIT_MULTITHREADED);

    BIND_CRASH_HANDLER_FOR_THREAD();

    Win32Handle clientConnectedEvent(::CreateEvent(NULL, TRUE, TRUE, NULL));
    if (!clientConnectedEvent.isValid()) {
        spdlog::error("CreateEvent for client connection failed {}", ::GetLastError());
        return 0;
    }

    while (!pThis->serviceExiting()) {
        OVERLAPPED overlapped;
        ::ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = clientConnectedEvent.getHandle();

        BOOL result = ::ConnectNamedPipe(pThis->clientPipe_.getHandle(), &overlapped);
        if (result == FALSE) {
            DWORD lastError = ::GetLastError();
            if ((lastError != ERROR_IO_PENDING) && (lastError != ERROR_PIPE_CONNECTED)) {
                spdlog::error("ConnectNamedPipe failed {}", lastError);
                break;
            }
        }

        DWORD dwWait = clientConnectedEvent.wait(INFINITE, TRUE);
        if (dwWait == WAIT_OBJECT_0) {
            pThis->processClientRequests();
            ::DisconnectNamedPipe(pThis->clientPipe_.getHandle());
        } else {
            if (dwWait == WAIT_FAILED) {
                spdlog::error("Client connect event wait failed {}", ::GetLastError());
            }
            break;
        }
    }

    // Have to release this singleton here as it uses COM and thus must clean up before we CoUninitialize().
    IcsManager::instance().release();

    ::CoUninitialize();

    return 0;
}

bool Helper::readAll(HANDLE hIOEvent, char *buf, DWORD len)
{
    char* dataBuf = buf;
    DWORD bytesToRead = len;

    while (bytesToRead > 0) {
        ::ZeroMemory(&overlapped_, sizeof(overlapped_));
        overlapped_.hEvent = hIOEvent;

        DWORD bytesRead = 0;
        auto result = ::ReadFile(clientPipe_.getHandle(), dataBuf, bytesToRead, &bytesRead, &overlapped_);
        if (result) {
            // The asynchronous read request could be satisfied immediately.
            bytesToRead -= bytesRead;
            dataBuf += bytesRead;
            continue;
        }

        auto lastError = ::GetLastError();
        if (lastError != ERROR_IO_PENDING) {
            spdlog::error("readAll ReadFile failed {}", lastError);
            return false;
        }

        result = ::GetOverlappedResultEx(clientPipe_.getHandle(), &overlapped_, &bytesRead, INFINITE, TRUE);
        if (result) {
            bytesToRead -= bytesRead;
            dataBuf += bytesRead;
            continue;
        }

        // An APC is queued to this thread, or its pending IO is cancelled (CancelIoEx), to instruct
        // it to stop.  No need to log a failure in those cases, or if the pipe has been closed by
        // the client.
        lastError = ::GetLastError();
        if (lastError != WAIT_IO_COMPLETION && lastError != ERROR_BROKEN_PIPE && lastError != ERROR_OPERATION_ABORTED) {
            spdlog::error("readAll GetOverlappedResultEx failed {}", lastError);
        }

        return false;
    }

    return true;
}

bool Helper::writeAll(HANDLE hIOEvent, const char *buf, DWORD len)
{
    const char* dataBuf = buf;
    DWORD bytesToWrite = len;

    while (bytesToWrite > 0) {
        ::ZeroMemory(&overlapped_, sizeof(overlapped_));
        overlapped_.hEvent = hIOEvent;

        DWORD bytesWritten = 0;
        auto result = ::WriteFile(clientPipe_.getHandle(), dataBuf, bytesToWrite, &bytesWritten, &overlapped_);
        if (result) {
            // The asynchronous write request could be satisfied immediately.
            bytesToWrite -= bytesWritten;
            dataBuf += bytesWritten;
            continue;
        }

        auto lastError = ::GetLastError();
        if (lastError != ERROR_IO_PENDING) {
            spdlog::error("writeAll WriteFile failed {}", lastError);
            return false;
        }

        result = ::GetOverlappedResultEx(clientPipe_.getHandle(), &overlapped_, &bytesWritten, INFINITE, TRUE);
        if (result) {
            bytesToWrite -= bytesWritten;
            dataBuf += bytesWritten;
            continue;
        }

        // An APC is queued to this thread, or its pending IO is cancelled (CancelIoEx), to instruct
        // it to stop.  No need to log a failure in those cases, or if the pipe has been closed by
        // the client.
        lastError = ::GetLastError();
        if (lastError != WAIT_IO_COMPLETION && lastError != ERROR_BROKEN_PIPE && lastError != ERROR_OPERATION_ABORTED) {
            spdlog::error("writeAll GetOverlappedResultEx failed {}", lastError);
        }

        return false;
    }

    return true;
}

} // end namespace wsl
