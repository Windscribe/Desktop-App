#pragma once

#include <QDateTime>
#include "imacaddresscontroller.h"
#include "engine/helper/helper.h"
#include "../networkdetectionmanager/networkdetectionmanager_mac.h"

class MacAddressController_mac : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_mac(QObject *parent, NetworkDetectionManager_mac *ndManager, Helper *helper);
    ~MacAddressController_mac() override;

    void initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing) override;

    void updateNetworkList();

private slots:
    void onPrimaryAdapterUp(const types::NetworkInterface &currentAdapter);
    void onPrimaryAdapterDown(const types::NetworkInterface &lastAdapter);
    void onPrimaryAdapterChanged(const types::NetworkInterface &currentAdapter);
    void onPrimaryAdapterNetworkLostOrChanged(const types::NetworkInterface &currentAdapter);
    void onNetworkListChanged(const QVector<types::NetworkInterface> &adapterList);
    void onWifiAdapterChanged(bool adapterUp);

private:
    Helper_mac *helper_;
    bool autoRotate_;
    bool actuallyAutoRotate_;
    int lastSpoofIndex_;
    QDateTime lastSpoofTime_;
    QList<QString> networksBeingUpdated_; // used to ignore network changes during an adapter reset
    types::MacAddrSpoofing macAddrSpoofing_;
    NetworkDetectionManager_mac *networkDetectionManager_;

    bool doneFilteringNetworkEvents();
    void autoRotateUpdateMacSpoof();
    types::MacAddrSpoofing macAddrSpoofingWithUpdatedNetworkList();
    void applyMacAddressSpoof(const QString &interfaceName, const QString &macAddress);
    void removeMacAddressSpoof(const QString &interfaceName);

    void applyAndRemoveSpoofs(QMap<QString, QString> spoofsToApply, QMap<QString, QString> spoofsToRemove);
    void removeSpoofs(QMap<QString, QString> spoofsToRemove);

    void filterAndRotateOrUpdateList();
};
