#pragma once

#include "inetworkdetectionmanager.h"
#include "networkchangeworkerthread.h"
#include "engine/helper/helper.h"
#include "types/networkinterface.h"

class NetworkDetectionManager_win : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_win(QObject *parent, Helper *helper);
    ~NetworkDetectionManager_win() override;

    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate = false) override;
    bool isOnline() override;

    bool interfaceEnabled(int interfaceIndex);
    void applyMacAddressSpoof(int ifIndex, QString macAddress);
    void removeMacAddressSpoof(int ifIndex);
    void resetAdapter(int ifIndex, bool bringBackUp = true);

private slots:
    void onNetworkChanged();

private:
    Helper *helper_;
    NetworkChangeWorkerThread *networkWorker_;
    types::NetworkInterface curNetworkInterface_;
    bool bLastIsOnline_;

    bool isOnlineImpl();

};
