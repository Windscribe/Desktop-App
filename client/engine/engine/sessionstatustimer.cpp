#include "sessionstatustimer.h"


SessionStatusTimer::SessionStatusTimer(QObject *parent, IConnectStateController *connectStateController) : QObject(parent)
{
    connect(connectStateController, &IConnectStateController::stateChanged, this, &SessionStatusTimer::onConnectStateChanged);
    connect(&timer_, &QTimer::timeout, this, &SessionStatusTimer::onTimer);
    timer_.setInterval(TIMER_INTERVAL);
}

void SessionStatusTimer::applicationActivated()
{
    if (timer_.isActive()) {
        timer_.start();
        requestSessionUpdate();
    }
}

void SessionStatusTimer::applicationDeactivated()
{
}

void SessionStatusTimer::start()
{
    // So we don't have to check for validity of this member in onTimer.
    lastSessionUpdateRequest_ = QDateTime::currentDateTimeUtc();
    timer_.start();
}

void SessionStatusTimer::stop()
{
    timer_.stop();
}

void SessionStatusTimer::onTimer()
{
    /*
    As specified by the 'Standardize Server API Interactions' document in the Hub, we fetch the session:
    - On app launch
      - The LoginController class handles this case.
    - Every 24 hours
      - Covered by the check-every-minute or check-every-hour cases.
    - Every hour
      - Covered by the check-every-minute case when connected.
    - Every minute when connected
    - When the app returns to the foreground (gets focus)
    */

    if (isConnected_) {
        requestSessionUpdate();
    }
    else if (lastSessionUpdateRequest_.secsTo(QDateTime::currentDateTimeUtc()) >= 60*60) {
        requestSessionUpdate();
    }
}

void SessionStatusTimer::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON /*reason*/, CONNECT_ERROR /*err*/, const LocationID & /*location*/)
{
    isConnected_ = (state == CONNECT_STATE_CONNECTED);
}

void SessionStatusTimer::requestSessionUpdate()
{
    emit needUpdateRightNow();
    lastSessionUpdateRequest_ = QDateTime::currentDateTimeUtc();
}
