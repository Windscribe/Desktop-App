#ifndef MACADDRESSCONTROLLER_WIN_H
#define MACADDRESSCONTROLLER_WIN_H

#include "imacaddresscontroller.h"
#include "../NetworkDetectionManager/networkdetectionmanager_win.h"
#include "Engine/Helper/ihelper.h"

class MacAddressController_win : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_win(QObject *parent, NetworkDetectionManager_win *ndManager);
    ~MacAddressController_win() override;

    void initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;

private slots:
    void onNetworkChange(bool isOnline, ProtoTypes::NetworkInterface networkInterface); // will fire on any network change

private:
    QList<int> networksBeingUpdated_; // used to ignore network changes during an adapter reset
    bool autoRotate_;
    bool actuallyAutoRotate_;

    ProtoTypes::MacAddrSpoofing macAddrSpoofing_;
    ProtoTypes::NetworkInterface lastNetworkInterface_;

    NetworkDetectionManager_win *networkDetectionManager_;

    void checkMacSpoofAppliedCorrectly();

    int lastSpoofIndex_;

};

#endif // MACADDRESSCONTROLLER_WIN_H
