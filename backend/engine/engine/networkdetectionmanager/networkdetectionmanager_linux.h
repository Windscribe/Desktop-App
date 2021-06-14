#ifndef NETWORKDETECTIONMANAGER_LINUX_H
#define NETWORKDETECTIONMANAGER_LINUX_H

#include <QMutex>
#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"

class NetworkDetectionManager_linux : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_linux(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_linux() override;
    void updateCurrentNetworkInterface(bool requested = false) override;
    bool isOnline() override;

signals:
    void networkChanged(ProtoTypes::NetworkInterface networkInterface); // remove once inherited from INetworkDetectionManager

};

#endif // NETWORKDETECTIONMANAGER_LINUX_H
