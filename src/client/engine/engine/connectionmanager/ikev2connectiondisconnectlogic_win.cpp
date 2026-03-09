#include "ikev2connectiondisconnectlogic_win.h"
#include "utils/crashhandler.h"
#include "utils/log/categories.h"

#include <QElapsedTimer>
#include <QProcess>

IKEv2ConnectionDisconnectLogic_win::IKEv2ConnectionDisconnectLogic_win(QObject *parent) : QThread(parent),
    connHandle_(NULL)
{
}

void IKEv2ConnectionDisconnectLogic_win::startDisconnect(HRASCONN connHandle)
{
    connHandle_ = connHandle;
    start(QThread::LowPriority);
}

bool IKEv2ConnectionDisconnectLogic_win::isDisconnected()
{
    return !isRunning();
}

void IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(HRASCONN connHandle)
{
    DWORD dwErr = RasHangUp(connHandle);
    qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), RasHangUp return code:" << dwErr;
    if (dwErr == ERROR_INVALID_HANDLE)
    {
        return;
    }
    else
    {
        QElapsedTimer elapsedTimer;
        elapsedTimer.start();

        while (true)
        {
            RASCONNSTATUS status;
            memset(&status, 0, sizeof(status));
            status.dwSize = sizeof(status);
            DWORD err = RasGetConnectStatus(connHandle, &status);
            if (err == ERROR_INVALID_HANDLE)
            {
                qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), RasGetConnectStatus return code:" << err << ", we disconnected";
                return;
            }
            else
            {

                if (elapsedTimer.elapsed() > 3000)
                {
                    qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), 3 sec elapsed:" << err;
                    qCInfo(LOG_IKEV2) << "Try console command: rasdial /DISCONNECT";
                    QProcess process;
                    process.start("rasdial", QStringList() << "/DISCONNECT");
                    process.waitForFinished();
                    return;
                }
                else
                {
                    Sleep(10);
                }
            }
        }
    }
}

void IKEv2ConnectionDisconnectLogic_win::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();

    DWORD dwErr = RasHangUp(connHandle_);
    qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::startDisconnect(), RasHangUp return code:" << dwErr;

    if (dwErr != ERROR_INVALID_HANDLE)
    {
        QElapsedTimer elapsedTimer;
        QElapsedTimer totalTimer;
        elapsedTimer.start();
        totalTimer.start();
        int cntRasHangUp = 1;
        bool rasdialDisconnectCalled = false;

        while (totalTimer.elapsed() < 15000)
        {
            RASCONNSTATUS status;
            memset(&status, 0, sizeof(status));
            status.dwSize = sizeof(status);
            DWORD err = RasGetConnectStatus(connHandle_, &status);

            if (err == ERROR_INVALID_HANDLE)
            {
                qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::run(), RasGetConnectStatus return code:" << err << ", we disconnected";
                break;
            }

            if (elapsedTimer.elapsed() > 3000)
            {
                if (cntRasHangUp < 3)
                {
                    err = RasHangUp(connHandle_);
                    elapsedTimer.restart();
                    qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::run(), call RasHangUp again:" << err;
                    cntRasHangUp++;
                }
                else if (!rasdialDisconnectCalled)
                {
                    qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::run(), 3 calls RasHangUp failed";
                    qCInfo(LOG_IKEV2) << "Try console command: rasdial /DISCONNECT";
                    QProcess process;
                    process.start("rasdial", QStringList() << "/DISCONNECT");
                    process.waitForFinished();
                    rasdialDisconnectCalled = true;
                    elapsedTimer.restart();
                }
            }

            Sleep(10);
        }

        if (totalTimer.elapsed() >= 15000)
        {
            qCInfo(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::run(), timed out waiting for disconnect, forcing disconnect";
        }
    }

    connHandle_ = NULL;
    emit disconnected();
}
