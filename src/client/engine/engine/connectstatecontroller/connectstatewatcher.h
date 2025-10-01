#pragma once

#include <QObject>
#include "iconnectstatecontroller.h"

// The utility class that is tracking the change of IConnectStateController states.
// Allows you to determine whether there was a change in the state of the VPN connection from the moment the object was created
// and until the moment the function isVpnConnectStateChanged() was called.
class ConnectStateWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ConnectStateWatcher(QObject *parent, IConnectStateController *connectStateController);

    bool isVpnConnectStateChanged() const;

private slots:
    void onConnectStateChanged();

private:
    IConnectStateController *connectStateController_;
    CONNECT_STATE initialState_;
    bool isVpnStateChanged_;
};
