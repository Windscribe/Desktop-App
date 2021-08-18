#include "connectstatecontroller.h"

ConnectStateController::ConnectStateController(QObject *parent) : IConnectStateController(parent),
    state_(CONNECT_STATE_DISCONNECTED),
    prevState_(CONNECT_STATE_DISCONNECTED),
    disconnectReason_(DISCONNECTED_ITSELF),
    err_(NO_CONNECT_ERROR),
    mutex_(QMutex::Recursive)
{
}

void ConnectStateController::setConnectedState(const LocationID &location)
{
    QMutexLocker locker(&mutex_);
    if (state_ != CONNECT_STATE_CONNECTED)
    {
        prevState_ = state_;
        state_ = CONNECT_STATE_CONNECTED;
        err_ = NO_CONNECT_ERROR;
        disconnectReason_ = DISCONNECTED_ITSELF;
        location_ = location;
        emit stateChanged(state_, disconnectReason_, err_, location_);
    }
}

void ConnectStateController::setDisconnectedState(DISCONNECT_REASON reason, CONNECTION_ERROR err)
{
    QMutexLocker locker(&mutex_);
    if (state_ != CONNECT_STATE_DISCONNECTED || reason == DISCONNECTED_WITH_ERROR)
    {
        prevState_ = state_;
        state_ = CONNECT_STATE_DISCONNECTED;
        disconnectReason_ = reason;
        err_ = err;
        location_ = LocationID();   //reset
        emit stateChanged(state_, disconnectReason_, err_, location_);
    }
}

void ConnectStateController::setConnectingState(const LocationID &location)
{
    QMutexLocker locker(&mutex_);
    if (state_ != CONNECT_STATE_CONNECTING)
    {
        prevState_ = state_;
        state_ = CONNECT_STATE_CONNECTING;
        err_ = NO_CONNECT_ERROR;
        disconnectReason_ = DISCONNECTED_ITSELF;
        location_ = location;
        emit stateChanged(state_, disconnectReason_, err_, location_);
    }
}

void ConnectStateController::setDisconnectingState()
{
    QMutexLocker locker(&mutex_);
    if (state_ != CONNECT_STATE_DISCONNECTING)
    {
        prevState_ = state_;
        state_ = CONNECT_STATE_DISCONNECTING;
        err_ = NO_CONNECT_ERROR;
        disconnectReason_ = DISCONNECTED_ITSELF;
        location_ = LocationID();   //reset
        emit stateChanged(state_, disconnectReason_, err_, location_);
    }
}

CONNECT_STATE ConnectStateController::currentState()
{
    QMutexLocker locker(&mutex_);
    return state_;
}

CONNECT_STATE ConnectStateController::prevState()
{
    QMutexLocker locker(&mutex_);
    return prevState_;
}

DISCONNECT_REASON ConnectStateController::disconnectReason()
{
    QMutexLocker locker(&mutex_);
    return disconnectReason_;
}

CONNECTION_ERROR ConnectStateController::connectionError()
{
    QMutexLocker locker(&mutex_);
    return err_;
}

const LocationID &ConnectStateController::locationId()
{
    QMutexLocker locker(&mutex_);
    return location_;
}
