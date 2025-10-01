#pragma once

#include <QDateTime>
#include "imacaddresscontroller.h"
#include "engine/helper/helper.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/networkdetectionmanager/networkdetectionmanager_linux.h"

class MacAddressController_linux : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_linux(QObject *parent, INetworkDetectionManager *ndManager, Helper *helper);
    ~MacAddressController_linux() override;

    void initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;

private slots:
    void onNetworkListChanged(const QList<types::NetworkInterface> &interfaces);

private:
    Helper *helper_;
    types::MacAddrSpoofing spoofing_;
    NetworkDetectionManager_linux *networkDetectionManager_;
    types::NetworkInterface lastInterface_;

    const int kAutoRotateMinimumTimeMs = 3000;

    void enableSpoofing(bool networkChanged = false);
    void disableSpoofing();
    void applySpoof(const types::NetworkInterface &interface, const QString &macAddress);
    void removeSpoofs();

#ifdef CLI_ONLY
    bool spoofInProgress_;
#endif
};
