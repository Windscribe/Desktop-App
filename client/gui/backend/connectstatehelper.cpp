#include "connectstatehelper.h"

ConnectStateHelper::ConnectStateHelper(QObject *parent) : QObject(parent),
    bInternalDisconnecting_(false), isDisconnected_(true)
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
    bInternalDisconnecting_ = true;
    isDisconnected_ = false;
    types::ConnectState cs;
    cs.connectState = CONNECT_STATE_DISCONNECTING;
    curState_ = cs;
    emit connectStateChanged(cs);
}

void ConnectStateHelper::setConnectStateFromEngine(const types::ConnectState &connectState)
{
    if (bInternalDisconnecting_)
    {
        if (connectState.connectState != CONNECT_STATE_DISCONNECTED)
        {
            return;
        }
        bInternalDisconnecting_ = false;
    }

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
