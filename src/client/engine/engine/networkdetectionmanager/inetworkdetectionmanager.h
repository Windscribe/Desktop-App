#pragma once

#include <QObject>
#include "types/networkinterface.h"

class INetworkDetectionManager : public QObject
{
    Q_OBJECT
public:
    explicit INetworkDetectionManager(QObject *parent) : QObject(parent) {}
    virtual ~INetworkDetectionManager() {}
    virtual void getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate = false) = 0;
    virtual bool isOnline() = 0;

signals:
    void networkChanged(const types::NetworkInterface &networkInterface);
    void onlineStateChanged(bool isOnline);

};
