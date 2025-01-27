#pragma once

#include <QObject>
#include "types/connectstate.h"

class ConnectStateHelper : public QObject
{
    Q_OBJECT
public:
    explicit ConnectStateHelper(QObject *parent = nullptr);

    void connectClickFromUser();
    void disconnectClickFromUser();
    void disconnect(DISCONNECT_REASON reason);
    void setConnectStateFromEngine(const types::ConnectState &connectState);
    bool isDisconnected() const;
    types::ConnectState currentConnectState() const;

signals:
    void connectStateChanged(const types::ConnectState &connectState);

private:
    bool isDisconnected_;
    types::ConnectState curState_;
};
