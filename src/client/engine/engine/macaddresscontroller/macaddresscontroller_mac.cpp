#include "macaddresscontroller_mac.h"

#include "utils/interfaceutils_mac.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"
#include "utils/network_utils/network_utils_mac.h"

const int MINIMUM_TIME_AUTO_SPOOF = 5000;


MacAddressController_mac::MacAddressController_mac(QObject *parent, NetworkDetectionManager_mac *ndManager, Helper *helper) : IMacAddressController (parent)
  , helper_(helper)
  , autoRotate_(false)
  , actuallyAutoRotate_(false)
  , lastSpoofIndex_(-1)
  , networkDetectionManager_(ndManager)
{
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::primaryAdapterUp, this, &MacAddressController_mac::onPrimaryAdapterUp);
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::primaryAdapterDown, this, &MacAddressController_mac::onPrimaryAdapterDown);
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::primaryAdapterChanged, this, &MacAddressController_mac::onPrimaryAdapterChanged);
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::primaryAdapterNetworkLostOrChanged, this, &MacAddressController_mac::onPrimaryAdapterNetworkLostOrChanged);
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::networkListChanged, this, &MacAddressController_mac::onNetworkListChanged);
    connect(networkDetectionManager_, &NetworkDetectionManager_mac::wifiAdapterChanged, this, &MacAddressController_mac::onWifiAdapterChanged);

    lastSpoofTime_ = QDateTime::currentDateTime();
}

MacAddressController_mac::~MacAddressController_mac()
{

}

void MacAddressController_mac::initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_) {
        if (macAddrSpoofing.isEnabled) {
            autoRotate_ = macAddrSpoofing.isAutoRotate;
        }
        macAddrSpoofing_ = macAddrSpoofing;
    }
}

void MacAddressController_mac::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_) {
        qCInfo(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing MacAddrSpoofing has changed. actuallyAutoRotate_=" << actuallyAutoRotate_;
        qCInfo(LOG_BASIC) << "MacAddrSpoofing settings:" << macAddrSpoofing.toJson(true);

        types::NetworkInterface currentAdapter = InterfaceUtils_mac::instance().currentNetworkInterface();
        types::NetworkInterface selectedInterface = macAddrSpoofing.selectedNetworkInterface;

        if (macAddrSpoofing.isEnabled) {
            QMap<QString, QString> spoofsToApply;
            QMap<QString, QString> spoofsToRemove;

            if (actuallyAutoRotate_) { // auto-rotate
                types::NetworkInterface lastInterface;
                networkDetectionManager_->getCurrentNetworkInterface(lastInterface);
                if (!types::NetworkInterface::isNoNetworkInterface(lastInterface.interfaceIndex)) {
                    lastSpoofIndex_ = lastInterface.interfaceIndex;
                    qCInfo(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Auto-rotating spoof on interface: " << lastInterface.interfaceIndex;
                    const QString macAddress(macAddrSpoofing.macAddress);

                    const QString lastInterfaceName = lastInterface.interfaceName;
                    if (NetworkUtils_mac::isAdapterUp(lastInterface.interfaceName)) { // apply-able interface
                        spoofsToApply.insert(lastInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                    } else {
                        qCWarning(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Can't auto-rotate - adapter not up";
                    }

                    // remove all but last
                    const QVector<types::NetworkInterface> spoofedInterfacesExceptLast = NetworkUtils::interfacesExceptOne(InterfaceUtils_mac::instance().currentSpoofedInterfaces(), lastInterface);
                    for (const types::NetworkInterface &networkInterface : spoofedInterfacesExceptLast) {
                        spoofsToRemove.insert(networkInterface.interfaceName, "");
                    }

                    actuallyAutoRotate_ = false;
                } else {
                    qCWarning(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Cannot Auto-rotate on No Interface";
                }
            } else { // Manual press or Update
                // apply spoof to current adapter if same as selected
                if (!macAddrSpoofing_.isEnabled ||
                    macAddrSpoofing.macAddress != macAddrSpoofing_.macAddress ||
                    selectedInterface.interfaceIndex != macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
                {
                    // apply spoof to current adapter
                    // changed enable state, mac address or interface
                    if (selectedInterface.interfaceIndex == currentAdapter.interfaceIndex) {
                        // valid adapter
                        if (!types::NetworkInterface::isNoNetworkInterface(currentAdapter.interfaceIndex)) {
                            lastSpoofIndex_ = currentAdapter.interfaceIndex;
                            const QString macAddress(macAddrSpoofing.macAddress);
                            const QString currentInterfaceName = currentAdapter.interfaceName;

                            qCInfo(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Manual spoof on interface: " << currentInterfaceName;

                            if (NetworkUtils_mac::isAdapterUp(currentAdapter.interfaceName)) { // apply-able interface
                                spoofsToApply.insert(currentInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                            } else {
                                qCWarning(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing dapter not up -- cannot apply spoof";
                            }

                            // remove all spoofs except current
                            const QVector<types::NetworkInterface> spoofedInterfacesExceptLast = NetworkUtils::interfacesExceptOne(InterfaceUtils_mac::instance().currentSpoofedInterfaces(), currentAdapter);
                            for (const types::NetworkInterface &networkInterface : spoofedInterfacesExceptLast) {
                                spoofsToRemove.insert(networkInterface.interfaceName, "");
                            }
                        } else {
                            qCWarning(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Can't spoof No Interface";
                        }
                    } else {
                        qCWarning(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Selected adapter is different than Current";
                    }
                } else {
                    qCInfo(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Non-triggering MacAddrSpoofing change";
                }
            }

            networksBeingUpdated_.clear();
            applyAndRemoveSpoofs(spoofsToApply, spoofsToRemove);
        } else {
            qCInfo(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing MAC spoofing is disabled";

            QMap<QString, QString> spoofsToRemove;
            const QVector<types::NetworkInterface> spoofs = InterfaceUtils_mac::instance().currentSpoofedInterfaces();
            for (const types::NetworkInterface &networkInterface : spoofs) {
                // only unspoof adapters that we spoofed, not from other sources
                if (NetworkUtils_mac::checkMacAddr(networkInterface.interfaceName, macAddrSpoofing_.macAddress)) {
                    spoofsToRemove.insert(networkInterface.interfaceName, "");
                }
            }

            networksBeingUpdated_.clear();
            removeSpoofs(spoofsToRemove);
        }

        // update state
        autoRotate_ = macAddrSpoofing.isAutoRotate;
        macAddrSpoofing_ = macAddrSpoofing;
    }
}

void MacAddressController_mac::updateNetworkList()
{
    types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofingWithUpdatedNetworkList();
    setMacAddrSpoofing(updatedMacAddrSpoofing);
    emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
}

void MacAddressController_mac::onPrimaryAdapterUp(const types::NetworkInterface &/*currentAdapter*/)
{
    if (doneFilteringNetworkEvents()) {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
    }

    qCDebug(LOG_BASIC) << "Set MAC spoofing (Up)";
    updateNetworkList();
}

void MacAddressController_mac::onPrimaryAdapterDown(const types::NetworkInterface &lastAdapter)
{
    Q_UNUSED(lastAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onPrimaryAdapterChanged(const types::NetworkInterface &currentAdapter)
{
    Q_UNUSED(currentAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onPrimaryAdapterNetworkLostOrChanged(const types::NetworkInterface &currentAdapter)
{
    Q_UNUSED(currentAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onNetworkListChanged(const QVector<types::NetworkInterface> & /*adapterList*/)
{
    if (doneFilteringNetworkEvents()) {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
    }

    qCDebug(LOG_BASIC) << "Set MAC Spoofing (List Changed)";
    updateNetworkList();
}

void MacAddressController_mac::onWifiAdapterChanged(bool adapterUp)
{
    Q_UNUSED(adapterUp);
    filterAndRotateOrUpdateList();
}

bool MacAddressController_mac::doneFilteringNetworkEvents()
{
    if (networksBeingUpdated_.size() == 0) {
        return true;
    }

    qCDebug(LOG_BASIC) << "Filtering MAC spoofing network events";
    QVector<types::NetworkInterface> networkInterfaces = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);
    for (const types::NetworkInterface &networkInterface : networkInterfaces) {
        if (networksBeingUpdated_.contains(networkInterface.interfaceName)) {
            if (NetworkUtils_mac::isAdapterUp(networkInterface.interfaceName)) {
                networksBeingUpdated_.removeAll(networkInterface.interfaceName);
            }
        }
    }

    return networksBeingUpdated_.size() == 0;
}

void MacAddressController_mac::autoRotateUpdateMacSpoof()
{
    types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofingWithUpdatedNetworkList();

    types::NetworkInterface lastInterface;
    networkDetectionManager_->getCurrentNetworkInterface(lastInterface);

    bool lastIsSameAsSelected = lastInterface.interfaceIndex == updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex;
    bool lastUp = NetworkUtils_mac::isAdapterUp(lastInterface.interfaceName);

    if (autoRotate_ && lastIsSameAsSelected && lastUp) { // apply-able interface
        qCInfo(LOG_BASIC) << "Auto-rotate generating MAC";
        QString newMacAddress = NetworkUtils::generateRandomMacAddress();
        updatedMacAddrSpoofing.macAddress = newMacAddress;
        actuallyAutoRotate_ = true;
    } else {
        qCInfo(LOG_BASIC) << "Couldn't rotate";
        qCInfo(LOG_BASIC) << "Auto rotate ON: " << autoRotate_;
        qCInfo(LOG_BASIC) << "Last is same as selected: " << lastIsSameAsSelected << " "
                           << lastInterface.interfaceName << " (last) "
                           << updatedMacAddrSpoofing.selectedNetworkInterface.interfaceName << "(selected)";
        qCInfo(LOG_BASIC) << "Last is up?: " << lastUp;
    }

    qCInfo(LOG_BASIC) << "Setting MAC Spoofing (Auto)";
    setMacAddrSpoofing(updatedMacAddrSpoofing);
    emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
}

types::MacAddrSpoofing MacAddressController_mac::macAddrSpoofingWithUpdatedNetworkList()
{
    types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
    QVector<types::NetworkInterface> networkInterfaces = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);

    // update adapter list
    updatedMacAddrSpoofing.networkInterfaces = networkInterfaces;

    // Check if the spoofed adapter is still in the list.
    bool found = false;
    for (const types::NetworkInterface &someInterface : networkInterfaces) {
        if (updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex == someInterface.interfaceIndex) {
            found = true;
            break;
        }
    }

    if (!found) {
        updatedMacAddrSpoofing.selectedNetworkInterface = types::NetworkInterface::noNetworkInterface();
    }

    return updatedMacAddrSpoofing;
}

void MacAddressController_mac::applyMacAddressSpoof(const QString &interfaceName, const QString &macAddress)
{
    if (macAddress.isEmpty()) {
        return;
    }

    bool success = true;
    helper_->setMacAddress(interfaceName, macAddress);
    emit macSpoofApplied();

    // Can verify MAC spoof immediately on Mac (no need to wait for adapter reset like on Windows).
    if (!NetworkUtils_mac::isInterfaceSpoofed(interfaceName) || !NetworkUtils_mac::checkMacAddr(interfaceName, macAddress)) {
        qCWarning(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof failed.";
        success = false;
    }

    if (success) {
        // Apply to boot script if successful
        lastSpoofTime_ = QDateTime::currentDateTime();
        qCInfo(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Successfully updated MAC address: " << interfaceName << " : " << macAddress;
        helper_->enableMacSpoofingOnBoot(true, interfaceName, macAddress);
    }
    else {
        qCWarning(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Was not able to spoof MAC-address on: " << interfaceName;
        emit sendUserWarning(USER_WARNING_MAC_SPOOFING_FAILURE_HARD);
    }
}

void MacAddressController_mac::removeMacAddressSpoof(const QString &interfaceName)
{
    helper_->setMacAddress(interfaceName, NetworkUtils_mac::trueMacAddress(interfaceName));
    emit macSpoofApplied();
}

void MacAddressController_mac::applyAndRemoveSpoofs(QMap<QString, QString> spoofsToApply, QMap<QString, QString> spoofsToRemove)
{
    // load filter list first so network listener can't empty list before we are done processing the updates
    const auto spoofToApplyKeys = spoofsToApply.keys();
    for (const QString &applyInterface : spoofToApplyKeys) {
        networksBeingUpdated_.append(applyInterface);
    }
    const auto spoofToRemoveKeys = spoofsToRemove.keys();
    for (const QString &removeInterface : spoofToRemoveKeys) {
        networksBeingUpdated_.append(removeInterface);
    }

    // apply spoofs -- should only be one max
    for (const QString &applyInterface : spoofToApplyKeys) {
        qCInfo(LOG_BASIC) << "Applying spoof: " << applyInterface << " " << spoofsToApply[applyInterface];
        applyMacAddressSpoof(applyInterface, spoofsToApply[applyInterface]);
    }
    // remove spoofs
    for (const QString &removeInterface : spoofToRemoveKeys) {
        qCInfo(LOG_BASIC) << "Removing spoof: " << removeInterface;
        removeMacAddressSpoof(removeInterface);
    }
}

void MacAddressController_mac::removeSpoofs(QMap<QString, QString> spoofsToRemove)
{
    // load filter list first so network listener can't empty list before we are done processing the updates
    const auto spoofsToRemoveKeys = spoofsToRemove.keys();
    for (const QString &removeInterface : spoofsToRemoveKeys) {
        networksBeingUpdated_.append(removeInterface);
    }
    for (const QString &removeInterface : std::as_const(networksBeingUpdated_)) {
        qCInfo(LOG_BASIC) << "Removing spoof: " << removeInterface;
        removeMacAddressSpoof(removeInterface);
    }
}

void MacAddressController_mac::filterAndRotateOrUpdateList()
{
    if (doneFilteringNetworkEvents()) {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
        if (lastSpoofTime_.msecsTo(QDateTime::currentDateTime()) > MINIMUM_TIME_AUTO_SPOOF ) {
            autoRotateUpdateMacSpoof();
        }
    } else {
        updateNetworkList();
    }
}
