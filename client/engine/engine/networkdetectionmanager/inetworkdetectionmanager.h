#ifndef INETWORKDETECTIONMANAGER_H
#define INETWORKDETECTIONMANAGER_H

#include <QObject>
#include "types/networkinterface.h"

class INetworkDetectionManager : public QObject
{
    Q_OBJECT
public:
    explicit INetworkDetectionManager(QObject *parent) : QObject(parent) {}
    virtual ~INetworkDetectionManager() {}
    virtual void getCurrentNetworkInterface(types::NetworkInterface &networkInterface) = 0;
    virtual bool isOnline() = 0;

signals:
    void networkChanged(const types::NetworkInterface &networkInterface);
    void onlineStateChanged(bool isOnline);

};

#endif // INETWORKDETECTIONMANAGER_H
