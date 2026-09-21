#pragma once

#include <memory>
#include <optional>

#include "engine/sleepevents/isleepevents.h"
#include "inetworkdetectionmanager.h"
#include "inetworkdetectionmanager_win_data_source.h"
#include "networkchangeworkerthread.h"
#include "types/networkinterface.h"

class NetworkDetectionManager_win : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_win(QObject *parent, ISleepEvents *sleepEvents);
    NetworkDetectionManager_win(QObject *parent, std::unique_ptr<INetworkDetectionManager_winDataSource> dataSource,
                                bool startWorker, ISleepEvents *sleepEvents);
    ~NetworkDetectionManager_win() override;

    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate = false) override;
    bool isOnline() override;

private slots:
    void onNetworkChanged();
    void onWake();

private:
    std::unique_ptr<INetworkDetectionManager_winDataSource> dataSource_;
    NetworkChangeWorkerThread *networkWorker_ = nullptr;
    types::NetworkInterface curNetworkInterface_;
    std::optional<QString> curNetworkId_;
    bool refreshedOnMissingId_ = false;
    bool bLastIsOnline_;
    bool needsPostWakeRefresh_ = false;

    bool isOnlineImpl();

};
