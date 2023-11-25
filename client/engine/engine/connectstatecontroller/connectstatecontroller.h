#pragma once

#include <QObject>
#include <QTimer>
#include <QMutex>
#include "types/enums.h"
#include "iconnectstatecontroller.h"

class ConnectStateController : public IConnectStateController
{
    Q_OBJECT
public:
    explicit ConnectStateController(QObject *parent);

    void setConnectedState(const LocationID &location);
    void setDisconnectedState(DISCONNECT_REASON reason, CONNECT_ERROR err);
    void setConnectingState(const LocationID &location);
    void setDisconnectingState();

    virtual CONNECT_STATE currentState() override;
    virtual CONNECT_STATE prevState() override;

    DISCONNECT_REASON disconnectReason() override;
    CONNECT_ERROR connectionError() override;
    const LocationID& locationId() override;

private:
    CONNECT_STATE state_;
    CONNECT_STATE prevState_;
    DISCONNECT_REASON disconnectReason_;
    CONNECT_ERROR err_;
    LocationID location_;
    QRecursiveMutex mutex_;
};
