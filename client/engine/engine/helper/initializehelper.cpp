#include "initializehelper.h"

#include <QTimer>

#include "engine/helper/ihelper.h"
#include "utils/log/categories.h"

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
    if (helper_->currentState() == IHelper::STATE_CONNECTED)
    {
        qCDebug(LOG_BASIC) << "Windscribe helper connected ok";
        timer->stop();
        timer->deleteLater();
        printHelperVersion();
        emit finished(INIT_HELPER_SUCCESS);
    }
    else if (helper_->currentState() == IHelper::STATE_FAILED_CONNECT || helper_->currentState() == IHelper::STATE_INSTALL_FAILED)
    {
        timer->stop();
        timer->deleteLater();
        qCWarning(LOG_BASIC) << "Windscribe helper connect failed";
        if (helperInitAttempts_ >= 1)
        {
            emit finished(INIT_HELPER_FAILED);
        }
        else
        {
            qCInfo(LOG_BASIC) << "Attempting to reinstall helper";
            if (helper_->reinstallHelper())
            {
                qCInfo(LOG_BASIC) << "Helper reinstalled";
                helper_->startInstallHelper();
                helperInitAttempts_++;
                QTimer *newtimer = new QTimer(this);
                connect(newtimer, &QTimer::timeout, this, &InitializeHelper::onTimerControlHelper);
                newtimer->start(10);
            }
            else
            {
                qCCritical(LOG_BASIC) << "Failed to reinstall helper";
                emit finished(INIT_HELPER_FAILED);
            }
        }
    }
    else if (helper_->currentState() == IHelper::STATE_USER_CANCELED)
    {
        timer->stop();
        timer->deleteLater();
        qCInfo(LOG_BASIC) << "Failed to install helper (user canceled)";
        emit finished(INIT_HELPER_USER_CANCELED);
    }
}

void InitializeHelper::handleHelperInit()
{
    if (helper_->currentState() == IHelper::STATE_CONNECTED)
    {
        qCDebug(LOG_BASIC) << "Windscribe helper connected ok";
        printHelperVersion();
        emit finished(INIT_HELPER_SUCCESS);
    }
    else
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &InitializeHelper::onTimerControlHelper);
        timer->start(10);
    }
}

void InitializeHelper::printHelperVersion()
{
    QString helperVersion = helper_->getHelperVersion();
    if (!helperVersion.isEmpty())
    {
        qCInfo(LOG_BASIC) << "Helper version" << helperVersion;
    }
}
