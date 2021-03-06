#ifndef NETWORKDETECTIONMANAGER_MAC_H
#define NETWORKDETECTIONMANAGER_MAC_H

#include <QMutex>
#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"

class NetworkDetectionManager_mac : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_mac(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_mac() override;
    void getCurrentNetworkInterface(ProtoTypes::NetworkInterface &networkInterface) override;
    bool isOnline() override;

signals:
    void networkListChanged(const ProtoTypes::NetworkInterfaces &networkInterfaces);
    void primaryAdapterUp(const ProtoTypes::NetworkInterface &currentAdapter);
    void primaryAdapterDown(const ProtoTypes::NetworkInterface &lastAdapter);
    void primaryAdapterChanged(const ProtoTypes::NetworkInterface &currentAdapter);
    void primaryAdapterNetworkLostOrChanged(const ProtoTypes::NetworkInterface &currentAdapter);
    void wifiAdapterChanged(bool wifiAdapterUp);

private slots:
    void onNetworkStateChanged();

private:
    IHelper *helper_;
    bool lastWifiAdapterUp_;
    bool lastIsOnlineState_;
    QMutex mutex_;
    ProtoTypes::NetworkInterface lastNetworkInterface_;
    ProtoTypes::NetworkInterfaces lastNetworkList_;

    bool isWifiAdapterUp(const ProtoTypes::NetworkInterfaces &networkList);
    const ProtoTypes::NetworkInterface currentNetworkInterfaceFromNetworkList(const ProtoTypes::NetworkInterfaces &networkList);

    bool isOnlineImpl();

};

#endif // NETWORKDETECTIONMANAGER_MAC_H
