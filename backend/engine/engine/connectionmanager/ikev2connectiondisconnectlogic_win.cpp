#include "ikev2connectiondisconnectlogic_win.h"
#include "Utils/logger.h"

#include <QProcess>

IKEv2ConnectionDisconnectLogic_win::IKEv2ConnectionDisconnectLogic_win(QObject *parent) : QThread(parent),
    cntRasHangUp_(0), connHandle_(NULL)
{
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
}

void IKEv2ConnectionDisconnectLogic_win::startDisconnect(HRASCONN connHandle)
{
    start(QThread::LowPriority);

    cntRasHangUp_ = 1;
    connHandle_ = connHandle;

    DWORD dwErr = RasHangUp(connHandle_);

    qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::startDisconnect(), RasHangUp return code:" << dwErr;
    if (dwErr == ERROR_INVALID_HANDLE)
    {
        connHandle_ = NULL;
        waitForControlThreadFinish();
        emit disconnected();
    }
    else
    {
        elapsedTimer_.start();
        timer_.start(10);
    }
}

bool IKEv2ConnectionDisconnectLogic_win::isDisconnected()
{
    return connHandle_ == NULL;
}

void IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(HRASCONN connHandle)
{
    DWORD dwErr = RasHangUp(connHandle);
    qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), RasHangUp return code:" << dwErr;
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
                qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), RasGetConnectStatus return code:" << err << ", we disconnected";
                return;
            }
            else
            {

                if (elapsedTimer.elapsed() > 3000)
                {
                    qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::blockingDisconnect(), 3 sec elapsed:" << err;
                    qCDebug(LOG_IKEV2) << "Try console command: rasdial /DISCONNECT";
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
    mutex_.lock();
    if (!waitCondition_.wait(&mutex_, 2000))
    {
        qCDebug(LOG_IKEV2) << "Try console command: rasdial /DISCONNECT";
        QProcess process;
        process.start("rasdial", QStringList() << "/DISCONNECT");
        process.waitForFinished();
    }
    mutex_.unlock();
}

void IKEv2ConnectionDisconnectLogic_win::onTimer()
{
    RASCONNSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwSize = sizeof(status);
    DWORD err = RasGetConnectStatus(connHandle_, &status);
    if (err == ERROR_INVALID_HANDLE)
    {
        waitForControlThreadFinish();
        qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::onTimer(), RasGetConnectStatus return code:" << err << ", we disconnected";
        timer_.stop();
        connHandle_ = NULL;
        emit disconnected();
    }
    else
    {
        // if 3 sec elapsed
        if (elapsedTimer_.elapsed() > 3000)
        {
            qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::onTimer(), 3 sec elapsed:" << err;

            if (cntRasHangUp_ < 3)
            {
                err = RasHangUp(connHandle_);
                elapsedTimer_.start();
                qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::onTimer(), call RasHangUp again:" << err;
                cntRasHangUp_++;
            }
            else
            {
                qCDebug(LOG_IKEV2) << "IKEv2ConnectionDisconnectLogic_win::onTimer(), 3 calls RasHangUp failed";
            }
        }
    }
}

void IKEv2ConnectionDisconnectLogic_win::waitForControlThreadFinish()
{
    mutex_.lock();
    waitCondition_.wakeAll();
    mutex_.unlock();
    wait();
}
