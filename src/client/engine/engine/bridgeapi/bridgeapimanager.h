#pragma once

#include <QMap>
#include <QObject>

#include <wsnet/WSNet.h>
#include "types/protocol.h"

class BridgeApiManager : public QObject
{
    Q_OBJECT

public:
    explicit BridgeApiManager(QObject *parent);
    virtual ~BridgeApiManager();

    void setConnectedState(bool isConnected, const QString &nodeAddress, const types::Protocol &protocol, const QString &pinnedIp = QString());

    void rotateIp();

signals:
    void ipPinFinished(const QString &ip);
    void ipRotateFinished(bool success);
    void apiAvailabilityChanged(bool isAvailable);

private:
    bool isConnected_;

    void pinIp(const QString &ip);
};
