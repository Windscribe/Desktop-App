#ifndef NETWORKSTATEMANAGER_WIN_H
#define NETWORKSTATEMANAGER_WIN_H

#include <Netlistmgr.h>
#include <QMutex>
#include "inetworkstatemanager.h"
#include "../NetworkDetectionManager/inetworkdetectionmanager.h"

// thread safe access
class NetworkStateManager_win : public INetworkStateManager
{
    Q_OBJECT
public:
    NetworkStateManager_win(QObject *parent, INetworkDetectionManager *networkDetectionManager);
    ~NetworkStateManager_win();

    bool isOnline() override;
    //void onlineStateChanged(bool connectivity);

private slots:
    void onDetectionManagerNetworkChanged(ProtoTypes::NetworkInterface networkInterface);
private:
    INetworkDetectionManager *pNetworkDetectionManager_;
    // INetworkListManager *pNetworkListManager_;
    // IConnectionPoint *pConnectPoint_;
    // DWORD cookie_;

    bool bIsOnline_;
    QMutex mutex_;

};

#endif // NETWORKSTATEMANAGER_WIN_H
