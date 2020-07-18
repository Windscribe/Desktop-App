#ifndef INETWORKDETECTIONMANAGER_H
#define INETWORKDETECTIONMANAGER_H

#include <QObject>
#include "ipc/generated_proto/types.pb.h"

class INetworkDetectionManager : public QObject
{
    Q_OBJECT
public:
    explicit INetworkDetectionManager(QObject *parent) : QObject(parent) {}
    virtual ~INetworkDetectionManager() {}
    virtual void updateCurrentNetworkInterface(bool requested) = 0;
    virtual bool isOnline() = 0;

signals:
    void networkChanged(ProtoTypes::NetworkInterface networkInterface);

};

#endif // INETWORKDETECTIONMANAGER_H
