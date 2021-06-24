#ifndef MACADDRESSCONTROLLER_MAC_H
#define MACADDRESSCONTROLLER_MAC_H

#include <QDateTime>
#include "imacaddresscontroller.h"
#include "engine/helper/helper_mac.h"
#include "../networkdetectionmanager/networkdetectionmanager_mac.h"

class MacAddressController_mac : public IMacAddressController
{
    Q_OBJECT
public:
    MacAddressController_mac(QObject *parent, NetworkDetectionManager_mac *ndManager, IHelper *helper);
    ~MacAddressController_mac() override;

    void initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;
    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing) override;

    void updateNetworkList();

private slots:
    void onPrimaryAdapterUp(const ProtoTypes::NetworkInterface &currentAdapter);
    void onPrimaryAdapterDown(const ProtoTypes::NetworkInterface &lastAdapter);
    void onPrimaryAdapterChanged(const ProtoTypes::NetworkInterface &currentAdapter);
    void onPrimaryAdapterNetworkLostOrChanged(const ProtoTypes::NetworkInterface &currentAdapter);
    void onNetworkListChanged(const ProtoTypes::NetworkInterfaces &adapterList);
    void onWifiAdapterChaged(bool adapterUp);

private:
    Helper_mac *helper_;
    bool autoRotate_;
    bool actuallyAutoRotate_;
    int lastSpoofIndex_;
    QDateTime lastSpoofTime_;
    QList<QString> networksBeingUpdated_; // used to ignore network changes during an adapter reset
    ProtoTypes::MacAddrSpoofing macAddrSpoofing_;
    NetworkDetectionManager_mac *networkDetectionManager_;

    bool doneFilteringNetworkEvents();
    void autoRotateUpdateMacSpoof();
    ProtoTypes::MacAddrSpoofing macAddrSpoofingWithUpdatedNetworkList();
    void applyMacAddressSpoof(const QString &interfaceName, const QString &macAddress);
    void removeMacAddressSpoof(const QString &interfaceName);

    void applyAndRemoveSpoofs(QMap<QString, QString> spoofsToApply, QMap<QString, QString> spoofsToRemove);
    void removeSpoofs(QMap<QString, QString> spoofsToRemove);

    void filterAndRotateOrUpdateList();
};

#endif // MACADDRESSCONTROLLER_MAC_H
