#include "connectstatehelper.h"

ConnectStateHelper::ConnectStateHelper(QObject *parent) : QObject(parent),
    isDisconnected_(true)
{

}

void ConnectStateHelper::connectClickFromUser()
{
    isDisconnected_ = false;
    types::ConnectState cs;
    cs.connectState = CONNECT_STATE_CONNECTING;
    curState_ = cs;
    emit connectStateChanged(cs);
}

void ConnectStateHelper::disconnectClickFromUser()
{
    isDisconnected_ = false;
    types::ConnectState cs;
    cs.connectState = CONNECT_STATE_DISCONNECTING;
    curState_ = cs;
    emit connectStateChanged(cs);
}

void ConnectStateHelper::setConnectStateFromEngine(const types::ConnectState &connectState)
{
    isDisconnected_ = connectState.connectState == CONNECT_STATE_DISCONNECTED;
    curState_ = connectState;
    emit connectStateChanged(connectState);
}

bool ConnectStateHelper::isDisconnected() const
{
    return isDisconnected_;
}

types::ConnectState ConnectStateHelper::currentConnectState() const
{
    return curState_;
}

void ConnectStateHelper::disconnect(DISCONNECT_REASON reason)
{
    isDisconnected_ = true;
    types::ConnectState cs;
    cs.connectState = CONNECT_STATE_DISCONNECTING;
    cs.disconnectReason = reason;
    curState_ = cs;
    emit connectStateChanged(cs);
}
