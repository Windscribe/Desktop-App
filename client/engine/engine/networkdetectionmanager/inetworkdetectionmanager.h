#ifndef INETWORKDETECTIONMANAGER_H
#define INETWORKDETECTIONMANAGER_H

#include <QObject>
#include "utils/protobuf_includes.h"

class INetworkDetectionManager : public QObject
{
    Q_OBJECT
public:
    explicit INetworkDetectionManager(QObject *parent) : QObject(parent) {}
    virtual ~INetworkDetectionManager() {}
    virtual void getCurrentNetworkInterface(ProtoTypes::NetworkInterface &networkInterface) = 0;
    virtual bool isOnline() = 0;

signals:
    void networkChanged(const ProtoTypes::NetworkInterface &networkInterface);
    void onlineStateChanged(bool isOnline);

};

#endif // INETWORKDETECTIONMANAGER_H
