#include "getmyipcontroller.h"
#include "engine/serverapi/serverapi.h"
#include <QTimer>

GetMyIPController::GetMyIPController(QObject *parent, ServerAPI *serverAPI, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    serverAPI_(serverAPI), networkDetectionManager_(networkDetectionManager), requestForTimerIsDisconnected_(false)
{
    connect(serverAPI_, SIGNAL(myIPAnswer(QString,bool,bool,uint)), SLOT(onMyIpAnswer(QString,bool,bool,uint)), Qt::QueuedConnection);
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
    timer_.setSingleShot(true);
    serverApiUserRole_ = serverAPI_->getAvailableUserRole();
}

void GetMyIPController::getIPFromConnectedState(int timeoutMs)
{
    if (timer_.isActive())
    {
        timer_.stop();
    }

    requestForTimerIsDisconnected_ = false;
    timer_.start(timeoutMs);
}

void GetMyIPController::getIPFromDisconnectedState(int timeoutMs)
{
    if (timer_.isActive())
    {
        timer_.stop();
    }

    requestForTimerIsDisconnected_ = true;
    timer_.start(timeoutMs);
}

void GetMyIPController::onTimer()
{
    if (networkDetectionManager_->isOnline())
    {
        serverAPI_->myIP(requestForTimerIsDisconnected_, serverApiUserRole_, true);
    }
    else
    {
        timer_.stop();
        timer_.start(1000);
    }
}

void GetMyIPController::onMyIpAnswer(const QString &ip, bool success, bool isDisconnected, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (!success)
        {
            timer_.stop();
            timer_.start(1000);
        }
        else
        {
            emit answerMyIP(ip, success, isDisconnected);
        }
    }
}
