#ifndef NETWORKDETECTIONMANAGER_WIN_H
#define NETWORKDETECTIONMANAGER_WIN_H

#include "inetworkdetectionmanager.h"
#include "networkchangeworkerthread.h"
#include "IPC/generated_proto/types.pb.h"
#include "Engine/Helper/ihelper.h"

class NetworkDetectionManager_win : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_win(QObject *parent, IHelper *helper);
    ~NetworkDetectionManager_win() override;

    void updateCurrentNetworkInterface(bool requested = false) override;
    bool isOnline() override;

    bool interfaceEnabled(int interfaceIndex);
    void applyMacAddressSpoof(int ifIndex, QString macAddress);
    void removeMacAddressSpoof(int ifIndex);
    void resetAdapter(int ifIndex, bool bringBackUp = true);

private slots:
    void onNetworkChanged();

private:
    IHelper *helper_;
    NetworkChangeWorkerThread *networkWorker_;
    ProtoTypes::NetworkInterface lastSentNetworkInterface_;

};

#endif // NETWORKDETECTIONMANAGER_WIN_H
