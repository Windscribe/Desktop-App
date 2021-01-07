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
    if (this_ && NotificationType == MibParameterNotification)
        this_->postUpdate();
}
}  // namespace


NetworkChangeWorkerThread::NetworkChangeWorkerThread(QObject *parent) : QThread(parent)
{
    hExitEvent_ = CreateEvent(NULL, false, false, NULL);
}

NetworkChangeWorkerThread::~NetworkChangeWorkerThread()
{
    CloseHandle(hExitEvent_);
}

void NetworkChangeWorkerThread::earlyExit()
{
    SetEvent(hExitEvent_);
}

void NetworkChangeWorkerThread::postUpdate()
{
    emit networkChanged();
}

void NetworkChangeWorkerThread::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    HANDLE hEvents[3];

    HANDLE hand1 = NULL;
    HANDLE hand2 = NULL;

    hEvents[0] = hExitEvent_;
    hEvents[1] = CreateEvent(NULL, false, false, NULL);
    hEvents[2] = CreateEvent(NULL, false, false, NULL);

    HANDLE ipInterfaceChangeCallbackHandle = nullptr;
    NotifyIpInterfaceChange(AF_UNSPEC, IpInterfaceChangeCallback, this,
        FALSE, &ipInterfaceChangeCallbackHandle);

    while (true)
    {
        OVERLAPPED overlap1;
        overlap1.hEvent = hEvents[1];
        DWORD ret = NotifyAddrChange(&hand1, &overlap1);
        if (ret != ERROR_IO_PENDING)
        {
            break;
        }

        OVERLAPPED overlap2;
        overlap2.hEvent = hEvents[2];
        ret = NotifyRouteChange(&hand2, &overlap2);
        if (ret != ERROR_IO_PENDING)
        {
            break;
        }

        DWORD dwWaitResult = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
        if ( dwWaitResult == WAIT_OBJECT_0 + 1 || dwWaitResult == WAIT_OBJECT_0 + 2)
        {
            postUpdate();
        }
        else
        {
            break;
        }
    }

    if (ipInterfaceChangeCallbackHandle)
        CancelMibChangeNotify2(ipInterfaceChangeCallbackHandle);

    CloseHandle(hEvents[1]);
    CloseHandle(hEvents[2]);
}
