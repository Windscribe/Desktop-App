#ifndef CONNECTSTATECONTROLLER_H
#define CONNECTSTATECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QRecursiveMutex>
#include "engine/types/types.h"
#include "iconnectstatecontroller.h"

class ConnectStateController : public IConnectStateController
{
    Q_OBJECT
public:
    explicit ConnectStateController(QObject *parent);

    void setConnectedState(const LocationID &location);
    void setDisconnectedState(DISCONNECT_REASON reason, ProtoTypes::ConnectError err);
    void setConnectingState(const LocationID &location);
    void setDisconnectingState();

    virtual CONNECT_STATE currentState() override;
    virtual CONNECT_STATE prevState() override;

    DISCONNECT_REASON disconnectReason() override;
    ProtoTypes::ConnectError connectionError() override;
    const LocationID& locationId() override;

private:
    CONNECT_STATE state_;
    CONNECT_STATE prevState_;
    DISCONNECT_REASON disconnectReason_;
    ProtoTypes::ConnectError err_;
    LocationID location_;
    QRecursiveMutex mutex_;
};

#endif // CONNECTSTATECONTROLLER_H
