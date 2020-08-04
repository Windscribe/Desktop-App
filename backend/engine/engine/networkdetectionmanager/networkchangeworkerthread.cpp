#include "networkchangeworkerthread.h"

#include <WinSock2.h>
#include <iphlpapi.h>
#include <Windows.h>

#include <QDebug>


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

void NetworkChangeWorkerThread::run()
{
    HANDLE hEvents[3];

    HANDLE hand1 = NULL;
    HANDLE hand2 = NULL;

    hEvents[0] = hExitEvent_;
    hEvents[1] = CreateEvent(NULL, false, false, NULL);
    hEvents[2] = CreateEvent(NULL, false, false, NULL);

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
            emit networkChanged();
        }
        else
        {
            break;
        }
    }

    CloseHandle(hEvents[1]);
    CloseHandle(hEvents[2]);
}
