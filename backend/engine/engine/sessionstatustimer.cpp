#include "sessionstatustimer.h"

#include <QDateTime>

SessionStatusTimer::SessionStatusTimer(QObject *parent, IConnectStateController *connectStateController) : QObject(parent),
    isStarted_(false), isApplicationActivated_(false), isConnected_(false), lastSignalEmitTimeMs_(0)
{
    connect(connectStateController, SIGNAL(stateChanged(CONNECT_STATE, DISCONNECT_REASON, CONNECTION_ERROR, LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE, DISCONNECT_REASON, CONNECTION_ERROR, LocationID)));
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
}

void SessionStatusTimer::applicationActivated()
{
    isApplicationActivated_ = true;

    if (isStarted_)
    {
        // if disconnected check last time emit signal > msec_
        if (!isConnected_)
        {
            qint64 curTime = QDateTime::currentMSecsSinceEpoch();
            if (lastSignalEmitTimeMs_ == 0 || ((curTime - lastSignalEmitTimeMs_) > msec_))
            {
                emit needUpdateRightNow();
                lastSignalEmitTimeMs_ = curTime;
            }
            if (!timer_.isActive())
            {
                timer_.start(TIMER_INTERVAL);
            }
        }
    }
}

void SessionStatusTimer::applicationDeactivated()
{
    isApplicationActivated_ = false;

    if (isStarted_)
    {
        if (!isConnected_)
        {
            timer_.stop();
        }
    }
}

void SessionStatusTimer::start(int msec)
{
    msec_ = msec;
    isStarted_ = true;
    lastSignalEmitTimeMs_ = QDateTime::currentMSecsSinceEpoch();

    if (isConnected_)
    {
        timer_.start(TIMER_INTERVAL);
    }
    else
    {
        if (isApplicationActivated_)
        {
            timer_.start(TIMER_INTERVAL);
        }
    }
}

void SessionStatusTimer::stop()
{
    isStarted_ = false;
    timer_.stop();
}

void SessionStatusTimer::onTimer()
{
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();
    if (lastSignalEmitTimeMs_ == 0 || ((curTime - lastSignalEmitTimeMs_) > msec_))
    {
        emit needUpdateRightNow();
        lastSignalEmitTimeMs_ = curTime;
    }
}

void SessionStatusTimer::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location)
{
    if (state == CONNECT_STATE_CONNECTED)
    {
        isConnected_ = true;
    }
    else
    {
        isConnected_ = false;
    }
    if (isStarted_)
    {
        if (isConnected_)
        {
            if (!timer_.isActive())
            {
                timer_.start(TIMER_INTERVAL);
            }
        }
        else
        {
            if (isApplicationActivated_)
            {
                if (!timer_.isActive())
                {
                    timer_.start(TIMER_INTERVAL);
                }
            }
        }
    }
}

