#include "networkchangeworkerthread.h"
#include "utils/crashhandler.h"

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <Windows.h>

#include <QDebug>

namespace
{
void __stdcall IpInterfaceChangeCallback(_In_ PVOID CallerContext,
                                         _In_ PMIB_IPINTERFACE_ROW /*Row*/ OPTIONAL,
                                         _In_ MIB_NOTIFICATION_TYPE NotificationType)
{
    auto *this_ = static_cast<NetworkChangeWorkerThread *>(CallerContext);
    if (this_ && NotificationType == MibParameterNotification) {
        this_->SignalIpInterfaceChange();
    }
}
}  // namespace


NetworkChangeWorkerThread::NetworkChangeWorkerThread(QObject *parent) : QThread(parent)
{
    hExitEvent_ = CreateEvent(NULL, false, false, NULL);
    hIpInterfaceChange_ = CreateEvent(NULL, false, false, NULL);
}

NetworkChangeWorkerThread::~NetworkChangeWorkerThread()
{
    if (hExitEvent_ != NULL) {
        CloseHandle(hExitEvent_);
    }

    if (hIpInterfaceChange_ != NULL) {
        CloseHandle(hIpInterfaceChange_);
    }
}

void NetworkChangeWorkerThread::earlyExit()
{
    if (hExitEvent_ != NULL) {
        SetEvent(hExitEvent_);
    }
}

void NetworkChangeWorkerThread::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    HANDLE hand1 = NULL;
    HANDLE hand2 = NULL;

    HANDLE hEvents[4];
    hEvents[0] = hExitEvent_;
    hEvents[1] = CreateEvent(NULL, false, false, NULL);
    hEvents[2] = CreateEvent(NULL, false, false, NULL);
    hEvents[3] = hIpInterfaceChange_;

    HANDLE ipInterfaceChangeCallbackHandle = nullptr;
    NotifyIpInterfaceChange(AF_UNSPEC, IpInterfaceChangeCallback, this,
                            FALSE, &ipInterfaceChangeCallbackHandle);

    OVERLAPPED overlap1;
    ZeroMemory(&overlap1, sizeof(overlap1));
    overlap1.hEvent = hEvents[1];

    OVERLAPPED overlap2;
    ZeroMemory(&overlap2, sizeof(overlap2));
    overlap2.hEvent = hEvents[2];

    // The NotifyAddrChange, NotifyRouteChange, and NotifyIpInterfaceChange events fire
    // multiple times during a network change event.  Introduce a small delay to minimize
    // the number of times we emit the networkChanged signal.
    DWORD dwTimeout = INFINITE;

    while (true)
    {
        DWORD ret = NotifyAddrChange(&hand1, &overlap1);
        if (ret != ERROR_IO_PENDING) {
            break;
        }

        ret = NotifyRouteChange(&hand2, &overlap2);
        if (ret != ERROR_IO_PENDING) {
            break;
        }

        ret = WaitForMultipleObjects(4, hEvents, FALSE, dwTimeout);
        if (ret == WAIT_TIMEOUT)
        {
            dwTimeout = INFINITE;
            emit networkChanged();
        }
        else if ((ret >= WAIT_OBJECT_0 + 1) && (ret <= WAIT_OBJECT_0 + 3)) {
            dwTimeout = 250;
        }
        else {
            break;
        }
    }

    if (ipInterfaceChangeCallbackHandle) {
        CancelMibChangeNotify2(ipInterfaceChangeCallbackHandle);
    }

    if (hand1 != NULL) {
        CancelIPChangeNotify(&overlap1);
    }

    if (hand2 != NULL) {
        CancelIPChangeNotify(&overlap2);
    }

    if (hEvents[1] != NULL) {
        CloseHandle(hEvents[1]);
    }

    if (hEvents[2] != NULL) {
        CloseHandle(hEvents[2]);
    }
}

void NetworkChangeWorkerThread::SignalIpInterfaceChange() const
{
    if (hIpInterfaceChange_) {
        SetEvent(hIpInterfaceChange_);
    }
}
