#ifndef ICONNECTSTATECONTROLLER_H
#define ICONNECTSTATECONTROLLER_H

#include <QObject>
#include <QTimer>
#include "engine/types/types.h"
#include "types/locationid.h"

class IConnectStateController : public QObject
{
    Q_OBJECT
public:
    explicit IConnectStateController(QObject *parent) : QObject(parent) {}
    virtual ~IConnectStateController() {}

    virtual CONNECT_STATE currentState() = 0;
    virtual CONNECT_STATE prevState() = 0;

    virtual DISCONNECT_REASON disconnectReason() = 0;
    virtual CONNECTION_ERROR connectionError() = 0;
    virtual const LocationID& locationId() = 0;

signals:
    void stateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location);
};

#endif // ICONNECTSTATECONTROLLER_H
