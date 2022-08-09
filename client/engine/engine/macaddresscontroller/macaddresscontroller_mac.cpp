#include "macaddresscontroller_mac.h"

#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/network_utils/network_utils_mac.h"

const int MINIMUM_TIME_AUTO_SPOOF = 5000;


MacAddressController_mac::MacAddressController_mac(QObject *parent, NetworkDetectionManager_mac *ndManager, IHelper *helper) : IMacAddressController (parent)
  , autoRotate_(false)
  , actuallyAutoRotate_(false)
  , lastSpoofIndex_(-1)
  , networkDetectionManager_(ndManager)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);
    connect(networkDetectionManager_, SIGNAL(primaryAdapterUp(types::NetworkInterface)), SLOT(onPrimaryAdapterUp(types::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterDown(types::NetworkInterface)), SLOT(onPrimaryAdapterDown(types::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterChanged(types::NetworkInterface)), SLOT(onPrimaryAdapterChanged(types::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterNetworkLostOrChanged(types::NetworkInterface)), SLOT(onPrimaryAdapterNetworkLostOrChanged(types::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(networkListChanged(QVector<types::NetworkInterface>)), SLOT(onNetworkListChanged(QVector<types::NetworkInterface>)));
    connect(networkDetectionManager_, SIGNAL(wifiAdapterChanged(bool)), SLOT(onWifiAdapterChaged(bool)));

    lastSpoofTime_ = QDateTime::currentDateTime();
}

MacAddressController_mac::~MacAddressController_mac()
{

}

void MacAddressController_mac::initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_)
    {
        if (macAddrSpoofing.isEnabled)
        {
            autoRotate_ = macAddrSpoofing.isAutoRotate;
        }

        macAddrSpoofing_ = macAddrSpoofing;
    }
}

void MacAddressController_mac::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_)
    {
        qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing MacAddrSpoofing has changed. actuallyAutoRotate_=" << actuallyAutoRotate_;
        qCDebug(LOG_BASIC) << macAddrSpoofing;

        types::NetworkInterface currentAdapter = NetworkUtils_mac::currentNetworkInterface();
        types::NetworkInterface selectedInterface = macAddrSpoofing.selectedNetworkInterface;

        if (macAddrSpoofing.isEnabled)
        {
            QMap<QString, QString> spoofsToApply;
            QMap<QString, QString> spoofsToRemove;

            if (actuallyAutoRotate_) // auto-rotate
            {
                types::NetworkInterface lastInterface;
                networkDetectionManager_->getCurrentNetworkInterface(lastInterface);
                int spoofIndex = lastInterface.interfaceIndex;
                if (spoofIndex != -1)
                {
                    lastSpoofIndex_ = spoofIndex;
                    qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Auto-rotating spoof on interface: " << spoofIndex;
                    const QString macAddress(macAddrSpoofing.macAddress);

                    const QString lastInterfaceName = lastInterface.interfaceName;
                    if (NetworkUtils_mac::isAdapterUp(lastInterface.interfaceName)) // apply-able interface
                    {
                        spoofsToApply.insert(lastInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                    }
                    else
                    {
                        qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Can't auto-rotate - adapter not up";
                    }

                    // remove all but last
                    const QVector<types::NetworkInterface> spoofedInterfacesExceptLast = Utils::interfacesExceptOne(NetworkUtils_mac::currentSpoofedInterfaces(), lastInterface);
                    for (const types::NetworkInterface &networkInterface : spoofedInterfacesExceptLast)
                    {
                        spoofsToRemove.insert(networkInterface.interfaceName, "");
                    }

                    actuallyAutoRotate_ = false;
                }
                else
                {
                    qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Cannot Auto-rotate on No Interface";
                }
            }
            else // Manual press or Update
            {
                // apply spoof to current adapter if same as selected
                if (!macAddrSpoofing_.isEnabled ||
                    macAddrSpoofing.macAddress != macAddrSpoofing_.macAddress ||
                    selectedInterface.interfaceIndex != macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
                {
                    // apply spoof to current adapter
                    // changed enable state, mac address or interface
                    if (selectedInterface.interfaceIndex == currentAdapter.interfaceIndex)
                    {
                        // valid adapter
                        if (currentAdapter.interfaceIndex != Utils::noNetworkInterface().interfaceIndex)
                        {
                            lastSpoofIndex_ = currentAdapter.interfaceIndex;
                            const QString macAddress(macAddrSpoofing.macAddress);
                            const QString currentInterfaceName = currentAdapter.interfaceName;

                            qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Manual spoof on interface: " << currentInterfaceName;

                            if (NetworkUtils_mac::isAdapterUp(currentAdapter.interfaceName)) // apply-able interface
                            {
                                spoofsToApply.insert(currentInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                            }
                            else
                            {
                                qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing dapter not up -- cannot apply spoof";
                            }

                            // remove all spoofs except current
                            const QVector<types::NetworkInterface> spoofedInterfacesExceptLast = Utils::interfacesExceptOne(NetworkUtils_mac::currentSpoofedInterfaces(), currentAdapter);
                            for (const types::NetworkInterface &networkInterface : spoofedInterfacesExceptLast)
                            {
                                spoofsToRemove.insert(networkInterface.interfaceName, "");
                            }
                        }
                        else
                        {
                            qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Can't spoof No Interface";
                        }
                    }
                    else
                    {
                        qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Selected adapter is different than Current";
                    }
                }
                else
                {
                    qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing Non-triggering MacAddrSpoofing change";
                }
            }

            networksBeingUpdated_.clear();
            applyAndRemoveSpoofs(spoofsToApply, spoofsToRemove);
        }
        else
        {
            qCDebug(LOG_BASIC) << "MacAddressController_mac::setMacAddrSpoofing MAC spoofing is disabled";

            QMap<QString, QString> spoofsToRemove;
            const QVector<types::NetworkInterface> spoofs = NetworkUtils_mac::currentSpoofedInterfaces();
            for (const types::NetworkInterface &networkInterface : spoofs)
            {
                spoofsToRemove.insert(networkInterface.interfaceName, "");
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
    if (doneFilteringNetworkEvents())
    {
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
    if (doneFilteringNetworkEvents())
    {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
    }

    qCDebug(LOG_BASIC) << "Set MAC Spoofing (List Changed)";
    updateNetworkList();
}

void MacAddressController_mac::onWifiAdapterChaged(bool adapterUp)
{
    Q_UNUSED(adapterUp);
    filterAndRotateOrUpdateList();
}

bool MacAddressController_mac::doneFilteringNetworkEvents()
{
    if (networksBeingUpdated_.size() == 0) return true;

    qCDebug(LOG_BASIC) << "Filtering MAC spoofing network events";
    QVector<types::NetworkInterface> networkInterfaces = NetworkUtils_mac::currentNetworkInterfaces(true);
    for (const types::NetworkInterface &networkInterface : networkInterfaces)
    {
        if (networksBeingUpdated_.contains(networkInterface.interfaceName))
        {
            if (NetworkUtils_mac::isAdapterUp(networkInterface.interfaceName))
            {
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

    if (autoRotate_ && lastIsSameAsSelected && lastUp) // apply-able interface
    {
        qCDebug(LOG_BASIC) << "Auto-rotate generating MAC";
        QString newMacAddress = Utils::generateRandomMacAddress();
        updatedMacAddrSpoofing.macAddress = newMacAddress;
        actuallyAutoRotate_ = true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Couldn't rotate";
        qCDebug(LOG_BASIC) << "Auto rotate ON: " << autoRotate_;
        qCDebug(LOG_BASIC) << "Last is same as selected: " << lastIsSameAsSelected << " "
                           << lastInterface.interfaceName << " (last) "
                           << updatedMacAddrSpoofing.selectedNetworkInterface.interfaceName << "(selected)";
        qCDebug(LOG_BASIC) << "Last is up?: " << lastUp;
    }

    qCDebug(LOG_BASIC) << "Setting MAC Spoofing (Auto)";
    setMacAddrSpoofing(updatedMacAddrSpoofing);
    emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
}

types::MacAddrSpoofing MacAddressController_mac::macAddrSpoofingWithUpdatedNetworkList()
{
    types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
    QVector<types::NetworkInterface> networkInterfaces = NetworkUtils_mac::currentNetworkInterfaces(true);

    // update adapter list
    updatedMacAddrSpoofing.networkInterfaces = networkInterfaces;

    // update selection
    bool found = false;
    for (const types::NetworkInterface &someInterface : networkInterfaces)
    {
        if (updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex != someInterface.interfaceIndex)
        {
            found = true;
        }
    }

    if (!found)
    {
        updatedMacAddrSpoofing.selectedNetworkInterface = Utils::noNetworkInterface();
    }

    return updatedMacAddrSpoofing;
}

void MacAddressController_mac::applyMacAddressSpoof(const QString &interfaceName, const QString &macAddress)
{
    QString command = "ifconfig " + interfaceName + " ether " + macAddress + " 2> /dev/null && echo success";
    QString result = helper_->executeRootCommand(command);

    bool success{ true };

    // Can verify MAC spoof immediately on Mac (no need to wait for adapter reset like on Windows).
    if (!NetworkUtils_mac::isInterfaceSpoofed(interfaceName) || !NetworkUtils_mac::checkMacAddr(interfaceName, macAddress))
    {
        // Check if ifconfig command faild
        if (result.contains("success"))
        {
            qCDebug(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Command \"" << command << "\" succeeded but MAC address not changed.";
        }
        else {
            qCDebug(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Command \"" << command << "\" failed.";
        }

        // Use heavier handed spoof command sequence, which may bring down the network interface temporarily
        robustMacAddressSpoof(interfaceName, macAddress);

        if (!NetworkUtils_mac::isInterfaceSpoofed(interfaceName) || !NetworkUtils_mac::checkMacAddr(interfaceName, macAddress))
        {
            qCDebug(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof \"robust\" failed too.";
            success = false;
        }
        else{
            success = true;
        }
    }

    if(success) {
        // Apply to boot script if successful
        lastSpoofTime_ = QDateTime::currentDateTime();
        qCDebug(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Successfully updated MAC address: " << interfaceName << " : " << macAddress;
        helper_->enableMacSpoofingOnBoot(true, interfaceName, macAddress);
    }
    else {
        qCDebug(LOG_BASIC) << "MacAddressController_mac::applyMacAddressSpoof Was not able to spoof MAC-address on: " << interfaceName;
        emit sendUserWarning(USER_WARNING_MAC_SPOOFING_FAILURE_HARD);
    }
}

void MacAddressController_mac::removeMacAddressSpoof(const QString &interfaceName)
{
    helper_->enableMacSpoofingOnBoot(false, "", "");
    QString originalMacAddress = NetworkUtils_mac::trueMacAddress(interfaceName);
    QString commandRemoveSpoofedMacAddress = "ifconfig " + interfaceName + " ether " + originalMacAddress;
    helper_->executeRootCommand(commandRemoveSpoofedMacAddress);

    // if MAC address was not reset to the original one, then try the heavy handed reset
    if (!NetworkUtils_mac::checkMacAddr(interfaceName, originalMacAddress))
    {
        robustMacAddressSpoof(interfaceName, originalMacAddress);
    }
}

void MacAddressController_mac::applyAndRemoveSpoofs(QMap<QString, QString> spoofsToApply, QMap<QString, QString> spoofsToRemove)
{
    // load filter list first so network listener can't empty list before we are done processing the updates
    const auto spoofToApplyKeys = spoofsToApply.keys();
    for (const QString &applyInterface : spoofToApplyKeys)
    {
        networksBeingUpdated_.append(applyInterface);
    }
    const auto spoofToRemoveKeys = spoofsToRemove.keys();
    for (const QString &removeInterface : spoofToRemoveKeys)
    {
        networksBeingUpdated_.append(removeInterface);
    }

    // apply spoofs -- should only be one max
    for (const QString &applyInterface : spoofToApplyKeys)
    {
        qCDebug(LOG_BASIC) << "Applying spoof: " << applyInterface << " " << spoofsToApply[applyInterface];
        applyMacAddressSpoof(applyInterface, spoofsToApply[applyInterface]);
    }
    // remove spoofs
    for (const QString &removeInterface : spoofToRemoveKeys)
    {
        qCDebug(LOG_BASIC) << "Removing spoof: " << removeInterface;
        removeMacAddressSpoof(removeInterface);
    }
}

void MacAddressController_mac::removeSpoofs(QMap<QString, QString> spoofsToRemove)
{
    // load filter list first so network listener can't empty list before we are done processing the updates
    const auto spoofsToRemoveKeys = spoofsToRemove.keys();
    for (const QString &removeInterface : spoofsToRemoveKeys)
    {
        networksBeingUpdated_.append(removeInterface);
    }
    for (const QString &removeInterface : qAsConst(networksBeingUpdated_))
    {
        qCDebug(LOG_BASIC) << "Removing spoof: " << removeInterface;
        removeMacAddressSpoof(removeInterface);
    }
}

void MacAddressController_mac::filterAndRotateOrUpdateList()
{
    if (doneFilteringNetworkEvents())
    {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
        if (lastSpoofTime_.msecsTo(QDateTime::currentDateTime()) > MINIMUM_TIME_AUTO_SPOOF )
        {
            autoRotateUpdateMacSpoof();
        }
    }
    else
    {
        updateNetworkList();
    }
}

void MacAddressController_mac::robustMacAddressSpoof(const QString &interfaceName, const QString &macAddress)
{
    // must be executed separately for some reason (otherwise the spoofed MAC address will not take)
    const QString robustCommand1 = "/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport -z";
    const QString robustCommand2 = "ifconfig " + interfaceName + " ether " + macAddress;
    const QString robustCommand3 = "ifconfig " + interfaceName + " up";

    helper_->executeRootCommand(robustCommand1);
    helper_->executeRootCommand(robustCommand2);
    helper_->executeRootCommand(robustCommand3);

    emit robustMacSpoofApplied();
}
