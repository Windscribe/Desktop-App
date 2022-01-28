#include "connectstatehelper.h"

ConnectStateHelper::ConnectStateHelper(QObject *parent) : QObject(parent),
    bInternalDisconnecting_(false), isDisconnected_(true)
{

}

void ConnectStateHelper::connectClickFromUser()
{
    isDisconnected_ = false;
    ProtoTypes::ConnectState cs;
    cs.set_connect_state_type(ProtoTypes::CONNECTING);
    curState_ = cs;
    emit connectStateChanged(cs);
}

void ConnectStateHelper::disconnectClickFromUser()
{
    bInternalDisconnecting_ = true;
    isDisconnected_ = false;
    ProtoTypes::ConnectState cs;
    cs.set_connect_state_type(ProtoTypes::DISCONNECTING);
    curState_ = cs;
    emit connectStateChanged(cs);
}

void ConnectStateHelper::setConnectStateFromEngine(const ProtoTypes::ConnectState &connectState)
{
    if (bInternalDisconnecting_)
    {
        if (connectState.connect_state_type() != ProtoTypes::DISCONNECTED)
        {
            return;
        }
        bInternalDisconnecting_ = false;
    }

    isDisconnected_ = connectState.connect_state_type() == ProtoTypes::DISCONNECTED;
    curState_ = connectState;
    emit connectStateChanged(connectState);
}

bool ConnectStateHelper::isDisconnected() const
{
    return isDisconnected_;
}

ProtoTypes::ConnectState ConnectStateHelper::currentConnectState() const
{
    return curState_;
}
