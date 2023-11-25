#pragma once

#include <QMutex>
#include "engine/helper/ihelper.h"
#include "inetworkdetectionmanager.h"

class NetworkDetectionManager_mac : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_mac(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_mac() override;
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface) override;
    bool isOnline() override;

signals:
    void networkListChanged(const QVector<types::NetworkInterface> &networkInterfaces);
    void primaryAdapterUp(const types::NetworkInterface &currentAdapter);
    void primaryAdapterDown(const types::NetworkInterface &lastAdapter);
    void primaryAdapterChanged(const types::NetworkInterface &currentAdapter);
    void primaryAdapterNetworkLostOrChanged(const types::NetworkInterface &currentAdapter);
    void wifiAdapterChanged(bool wifiAdapterUp);

private slots:
    void onNetworkStateChanged();

private:
    IHelper *helper_;
    bool lastWifiAdapterUp_;
    bool lastIsOnlineState_;
    QMutex mutex_;
    types::NetworkInterface lastNetworkInterface_;
    QVector<types::NetworkInterface> lastNetworkList_;

    bool isWifiAdapterUp(const QVector<types::NetworkInterface> &networkList);
    const types::NetworkInterface currentNetworkInterfaceFromNetworkList(const QVector<types::NetworkInterface> &networkList);

    bool isOnlineImpl();

};
