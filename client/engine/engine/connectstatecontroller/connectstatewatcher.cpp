#include "connectstatewatcher.h"


ConnectStateWatcher::ConnectStateWatcher(QObject *parent, IConnectStateController *connectStateController) :
    QObject(parent),
    connectStateController_(connectStateController),
    isVpnStateChanged_(false)
{
    initialState_ = connectStateController->currentState();
    connect(connectStateController, &IConnectStateController::stateChanged, this, &ConnectStateWatcher::onConnectStateChanged);
}

bool ConnectStateWatcher::isVpnConnectStateChanged() const
{
    return isVpnStateChanged_;
}

void ConnectStateWatcher::onConnectStateChanged()
{
    // already detected the VPN state change
    if (isVpnStateChanged_)
        return;

    CONNECT_STATE curState = connectStateController_->currentState();
    if (initialState_ == CONNECT_STATE_DISCONNECTED)
        isVpnStateChanged_ = (curState == CONNECT_STATE_CONNECTED || curState == CONNECT_STATE_DISCONNECTING);
    else if (initialState_ == CONNECT_STATE_CONNECTING)
        isVpnStateChanged_ = (curState == CONNECT_STATE_CONNECTED || curState == CONNECT_STATE_DISCONNECTING);
    else if (initialState_ == CONNECT_STATE_CONNECTED)
        isVpnStateChanged_ = (curState == CONNECT_STATE_DISCONNECTED || curState == CONNECT_STATE_CONNECTING);
    else if (initialState_ == CONNECT_STATE_DISCONNECTING)
        isVpnStateChanged_ = true;
}
