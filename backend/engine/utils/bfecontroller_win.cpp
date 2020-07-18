#include "bfecontroller_win.h"
#include "Engine/Helper/ihelper.h"
#include "Utils/winutils.h"
#include <windows.h>
#include "Utils/logger.h"
#include <QCoreApplication>
#include <QElapsedTimer>

bool BFEController_win::isBFEEnabled()
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        DWORD err = GetLastError();
        qCDebug(LOG_BASIC) << "OpenSCManager failed: " << err;
        return false;
    }

    schService = OpenService(schSCManager, L"BFE", SERVICE_QUERY_STATUS);
    if (schService == NULL)
    {
        DWORD err = GetLastError();
        qCDebug(LOG_BASIC) << "OpenService for BFE failed: " << err;
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;
    if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO,
                (LPBYTE) &ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ))
    {
        qCDebug(LOG_BASIC) << "QueryServiceStatusEx for BFE failed: " << GetLastError();
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    bool bRet = ssStatus.dwCurrentState == SERVICE_RUNNING;
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return bRet;
}

void BFEController_win::enableBFE(IHelper *helper)
{
    QString strLog = helper->enableBFE();
    qCDebug(LOG_BASIC) << "Enable BFE; Answer:" << strLog;
}

bool BFEController_win::checkAndEnableBFE(IHelper *helper)
{
    bool bBFEisRunning = isBFEEnabled();
    qCDebug(LOG_BASIC) << "Base filtering platform service is running:" << bBFEisRunning;
    if (!bBFEisRunning)
    {
        while (1)
        {
            enableBFE(helper);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();

            while (elapsedTimer.elapsed() < 3000)
            {
                QCoreApplication::processEvents();
                Sleep(1);

                if (isBFEEnabled())
                {
                    return true;
                }
            }
        }
    }

    return true;
}

BFEController_win::BFEController_win()
{

}
