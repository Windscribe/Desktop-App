#include "windscribehelper.h"

#include <sddl.h>

#include <spdlog/spdlog.h>

#include "active_processes.h"
#include "changeics/icsmanager.h"
#include "dns_firewall.h"
#include "firewallfilter.h"
#include "firewallonboot.h"
#include "ioutils.h"
#include "ipv6_firewall.h"
#include "openvpncontroller.h"
#include "process_command.h"
#include "split_tunneling/split_tunneling.h"
#include "utils.h"
#include "utils/crashhandler.h"

namespace wsl
{

static void stopClientHandlerThreadApc(ULONG_PTR lpParam)
{
    // No-op function queued to the client handler thread's APC queue to signal it to exit.
    // This will break the client handler thread out of its wait for a client pipe connection
    // or client data to arrive on the pipe.
    (void)lpParam;
}

WindscribeHelper::WindscribeHelper() : Win32Service(L"WindscribeService")
{
}

WindscribeHelper::~WindscribeHelper()
{
}

bool WindscribeHelper::onInit()
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

void WindscribeHelper::runService()
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

void WindscribeHelper::onStopRequest()
{
    stop();
}

void WindscribeHelper::onWindowsShutdown()
{
    spdlog::info("Windows shutdown notification received");
    stop();
}

void WindscribeHelper::onUserControl(DWORD controlCode)
{
    (void)controlCode;
}

void WindscribeHelper::stop()
{
    // Inform the SCM that we are processing the stop/shutdown request.
    setStatus(SERVICE_STOP_PENDING, 1000);
    if (clientHandlerThread_.isValid()) {
        ::QueueUserAPC(stopClientHandlerThreadApc, clientHandlerThread_.getHandle(), 0);
    }
}

bool WindscribeHelper::createClientPipe()
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;

    const TCHAR * szSD = TEXT("D:")   // Discretionary ACL
        TEXT("(D;OICI;GA;;;AN)")      // Deny access to anonymous logon
        TEXT("(A;OICI;GRGWGX;;;BG)")  // Allow access to built-in guests
        TEXT("(A;OICI;GRGWGX;;;AU)")  // Allow read/write/execute to authenticated users
        TEXT("(A;OICI;GA;;;BA)");     // Allow full control to administrators

    BOOL result = ::ConvertStringSecurityDescriptorToSecurityDescriptor(szSD, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL);
    if (!result) {
        return false;
    }

    clientPipe_.setHandle(::CreateNamedPipe(L"\\\\.\\pipe\\WindscribeService", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                            1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, &sa));
    if (!clientPipe_.isValid()) {
        spdlog::error("Failed to create the client commands pipe: {}", ::GetLastError());
        return false;
    }

    return true;
}

void WindscribeHelper::sendRequestResult(HANDLE hIOEvent, const std::string &data)
{
    // first 4 bytes - size of buffer
    const auto sizeOfBuf = static_cast<unsigned long>(data.size());
    if (IOUtils::writeAll(clientPipe_.getHandle(), hIOEvent, (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
        if (sizeOfBuf > 0) {
            IOUtils::writeAll(clientPipe_.getHandle(), hIOEvent, data.c_str(), sizeOfBuf);
        }
    }
}

void WindscribeHelper::processClientRequests()
{
    if (!Utils::verifyWindscribeProcessPath(clientPipe_.getHandle())) {
        return;
    }

    Win32Handle ioCompletedEvent(::CreateEvent(NULL, TRUE, TRUE, NULL));
    if (!ioCompletedEvent.isValid()) {
        spdlog::error("CreateEvent for client IPC IO completion failed: {}", ::GetLastError());
        return;
    }

    while (!serviceExiting()) {
        int cmdId;
        if (!IOUtils::readAll(clientPipe_.getHandle(), ioCompletedEvent.getHandle(), (char *)&cmdId, sizeof(cmdId))) {
            break;
        }

        unsigned long sizeOfBuf;
        if (!IOUtils::readAll(clientPipe_.getHandle(), ioCompletedEvent.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
            break;
        }

        std::string strData;
        if (sizeOfBuf > 0) {
            std::vector<char> buffer(sizeOfBuf);
            if (!IOUtils::readAll(clientPipe_.getHandle(), ioCompletedEvent.getHandle(), buffer.data(), sizeOfBuf)) {
                break;
            }
            strData = std::string(buffer.begin(), buffer.end());
        }

        auto result = processCommand((HelperCommand)cmdId, strData);
        sendRequestResult(ioCompletedEvent.getHandle(), result);
        ::FlushFileBuffers(clientPipe_.getHandle());
    }
}

DWORD WINAPI WindscribeHelper::clientHandlerThreadProc(LPVOID lpParameter)
{
    WindscribeHelper* pThis = static_cast<WindscribeHelper*>(lpParameter);

    // Required for the IcsManager class.
    ::CoInitializeEx(0, COINIT_MULTITHREADED);

    BIND_CRASH_HANDLER_FOR_THREAD();

    Win32Handle clientConnectedEvent(::CreateEvent(NULL, TRUE, TRUE, NULL));
    if (!clientConnectedEvent.isValid()) {
        spdlog::error("CreateEvent for client connection failed {}", ::GetLastError());
        return 0;
    }

    while (true) {
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

} // end namespace wsl
