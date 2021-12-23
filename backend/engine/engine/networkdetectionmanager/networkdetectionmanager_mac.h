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
    void networkChanged(bool isOnline, const ProtoTypes::NetworkInterface &networkInterface); // remove once inherited from INetworkDetectionManager
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
    QMutex mutex_;
    ProtoTypes::NetworkInterface lastNetworkInterface_;
    ProtoTypes::NetworkInterfaces lastNetworkList_;
    bool checkOnline(QString &networkInterface);
};

#endif // NETWORKDETECTIONMANAGER_MAC_H
