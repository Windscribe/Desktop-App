#include "initializehelper.h"

#include <QTimer>

#include "utils/log/categories.h"

InitializeHelper::InitializeHelper(QObject *parent, IHelperBackend *helperBackend) : QObject(parent),
    helperBackend_(helperBackend), helperInitAttempts_(0)
{
}

void InitializeHelper::start()
{
    QTimer::singleShot(0, this, SLOT(handleHelperInit()));
}

void InitializeHelper::onTimerControlHelper()
{
    QTimer *timer = (QTimer *)sender();
    if (helperBackend_->currentState() == IHelperBackend::State::kConnected)
    {
        qCDebug(LOG_BASIC) << "Windscribe helper connected ok";
        timer->stop();
        timer->deleteLater();
        emit finished(INIT_HELPER_SUCCESS);
    }
    else if (helperBackend_->currentState() == IHelperBackend::State::kFailedConnect || helperBackend_->currentState() == IHelperBackend::State::kInstallFailed)
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
            if (helperBackend_->reinstallHelper())
            {
                qCInfo(LOG_BASIC) << "Helper reinstalled";
                helperBackend_->startInstallHelper();
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
    else if (helperBackend_->currentState() == IHelperBackend::State::kUserCanceled)
    {
        timer->stop();
        timer->deleteLater();
        qCInfo(LOG_BASIC) << "Failed to install helper (user canceled)";
        emit finished(INIT_HELPER_USER_CANCELED);
    }
}

void InitializeHelper::handleHelperInit()
{
    if (helperBackend_->currentState() == IHelperBackend::State::kConnected)
    {
        qCDebug(LOG_BASIC) << "Windscribe helper connected ok";
        emit finished(INIT_HELPER_SUCCESS);
    }
    else
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &InitializeHelper::onTimerControlHelper);
        timer->start(10);
    }
}

