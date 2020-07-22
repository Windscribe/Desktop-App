#ifndef NETWORKDETECTIONMANAGER_MAC_H
#define NETWORKDETECTIONMANAGER_MAC_H

// TODO: remove once inherited from INetworkDetectionManager
#include <QObject>
#include "../../Common/IPC/generated_proto/types.pb.h"

// #include "inetworkdetectionmanager.h"

// TODO: detect network change
// TODO: grab priority network
// TODO: grab ethernet network name
// TODO: grab wifi ssid

class NetworkDetectionManager_mac : public QObject // public INetworkDetectionManager
{
    Q_OBJECT
public:
    explicit NetworkDetectionManager_mac(QObject *parent);
    ~NetworkDetectionManager_mac(); // override

    const ProtoTypes::NetworkInterface currentNetworkInterface(); // override

    void updateCurrentNetworkInterface(); // override

signals: // remove once inherited from INetworkDetectionManager
    void networkChanged(ProtoTypes::NetworkInterface networkInterface);

};

#endif // NETWORKDETECTIONMANAGER_MAC_H
