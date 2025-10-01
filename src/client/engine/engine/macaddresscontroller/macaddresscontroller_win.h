#pragma once

#include "imacaddresscontroller.h"
#include "../NetworkDetectionManager/networkdetectionmanager_win.h"

class MacAddressController_win : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_win(QObject *parent, NetworkDetectionManager_win *ndManager);
    ~MacAddressController_win() override;

    void initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;

private slots:
    void onNetworkChange(types::NetworkInterface networkInterface); // will fire on any network change

private:
    QList<int> networksBeingUpdated_; // used to ignore network changes during an adapter reset
    bool autoRotate_ = false;
    bool actuallyAutoRotate_ = false;
    int lastSpoofIndex_ = -1;

    types::MacAddrSpoofing macAddrSpoofing_;
    types::NetworkInterface lastNetworkInterface_;

    NetworkDetectionManager_win* const networkDetectionManager_;

    void checkMacSpoofAppliedCorrectly();
};
