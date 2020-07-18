#include "initializehelper.h"
#include "engine/helper/ihelper.h"
#include "utils/logger.h"

#include <QTimer>

InitializeHelper::InitializeHelper(QObject *parent, IHelper *helper) : QObject(parent),
    helper_(helper), helperInitAttempts_(0)
{
}

void InitializeHelper::start()
{
    QTimer::singleShot(0, this, SLOT(handleHelperInit()));
}

void InitializeHelper::onTimerControlHelper()
{
    QTimer *timer = (QTimer *)sender();
    if (helper_->isHelperConnected())
    {
        qCDebug(LOG_BASIC) << "OpenVPN helper connected ok";
        timer->stop();
        timer->deleteLater();
        printHelperVersion();
        emit finished(INIT_HELPER_SUCCESS);
    }
    else if (helper_->isFailedConnectToHelper())
    {
        timer->stop();
        timer->deleteLater();
        qCDebug(LOG_BASIC) << "OpenVPN helper connect failed";
        if (helperInitAttempts_ >= 1)
        {
            emit finished(INIT_HELPER_FAILED);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Attempt to reinstall helper";
            if (helper_->reinstallHelper())
            {
                qCDebug(LOG_BASIC) << "Helper reinstalled";
                helper_->startInstallHelper();
                helperInitAttempts_++;
                QTimer *timer = new QTimer(this);
                connect(timer, SIGNAL(timeout()), SLOT(onTimerControlHelper()));
                timer->start(10);
            }
            else
            {
                qCDebug(LOG_BASIC) << "Failed reinstall helper";
                emit finished(INIT_HELPER_FAILED);
            }
        }
    }
}

void InitializeHelper::handleHelperInit()
{
    if (helper_->isHelperConnected())
    {
        qCDebug(LOG_BASIC) << "OpenVPN helper connected ok";
        printHelperVersion();
        //qCDebug(LOG_BASIC) << "Remove windscribe urls from hosts file";
        //helper_->removeWindscribeUrlsFromHosts();
        emit finished(INIT_HELPER_SUCCESS);
    }
    else
    {
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), SLOT(onTimerControlHelper()));
        timer->start(10);
    }
}

void InitializeHelper::printHelperVersion()
{
    QString helperVersion = helper_->getHelperVersion();
    if (!helperVersion.isEmpty())
    {
        qCDebug(LOG_BASIC) << "Helper version" << helperVersion;
    }
}
