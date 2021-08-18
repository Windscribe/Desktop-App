#ifndef CONNECTSTATECONTROLLER_H
#define CONNECTSTATECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include "engine/types/types.h"
#include "iconnectstatecontroller.h"

class ConnectStateController : public IConnectStateController
{
    Q_OBJECT
public:
    explicit ConnectStateController(QObject *parent);

    void setConnectedState(const LocationID &location);
    void setDisconnectedState(DISCONNECT_REASON reason, CONNECTION_ERROR err);
    void setConnectingState(const LocationID &location);
    void setDisconnectingState();

    virtual CONNECT_STATE currentState() override;
    virtual CONNECT_STATE prevState() override;

    DISCONNECT_REASON disconnectReason() override;
    CONNECTION_ERROR connectionError() override;
    const LocationID& locationId() override;

private:
    CONNECT_STATE state_;
    CONNECT_STATE prevState_;
    DISCONNECT_REASON disconnectReason_;
    CONNECTION_ERROR err_;
    LocationID location_;
    QMutex mutex_;
};

#endif // CONNECTSTATECONTROLLER_H
