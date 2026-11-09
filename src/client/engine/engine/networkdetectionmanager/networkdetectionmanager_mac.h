#pragma once

#include <QMutex>
#include "engine/helper/helper.h"
#include "inetworkdetectionmanager.h"
#include "pathmonitor.h"

class NetworkDetectionManager_mac : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_mac(QObject *parent, Helper *helper);
    ~NetworkDetectionManager_mac() override;
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate = false) override;
    bool isOnline() override;

private slots:
    void onNetworkStateChanged();
    void onOnlineStateChanged(bool isOnline);

private:
    std::atomic<bool> lastIsOnlineState_;
    types::NetworkInterface lastNetworkInterface_;

    PathMonitor pathMonitor_;

    const types::NetworkInterface currentNetworkInterfaceFromNetworkList(const QVector<types::NetworkInterface> &networkList);
};
