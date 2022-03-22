#ifndef NETWORKDETECTIONMANAGER_WIN_H
#define NETWORKDETECTIONMANAGER_WIN_H

#include "inetworkdetectionmanager.h"
#include "networkchangeworkerthread.h"
#include "engine/helper/helper_win.h"

class NetworkDetectionManager_win : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_win(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_win() override;

    void getCurrentNetworkInterface(ProtoTypes::NetworkInterface &networkInterface) override;
    bool isOnline() override;

    bool interfaceEnabled(int interfaceIndex);
    void applyMacAddressSpoof(int ifIndex, QString macAddress);
    void removeMacAddressSpoof(int ifIndex);
    void resetAdapter(int ifIndex, bool bringBackUp = true);

private slots:
    void onNetworkChanged();

private:
    Helper_win *helper_;
    NetworkChangeWorkerThread *networkWorker_;
    ProtoTypes::NetworkInterface curNetworkInterface_;
    bool bLastIsOnline_;

    bool isOnlineImpl();

};

#endif // NETWORKDETECTIONMANAGER_WIN_H
