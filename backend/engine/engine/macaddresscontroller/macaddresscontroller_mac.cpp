#include "macaddresscontroller_mac.h"

#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/macutils.h"

const int MINIMUM_TIME_AUTO_SPOOF = 5000;


MacAddressController_mac::MacAddressController_mac(QObject *parent, NetworkDetectionManager_mac *ndManager, IHelper *helper) : IMacAddressController (parent)
  , autoRotate_(false)
  , actuallyAutoRotate_(false)
  , lastSpoofIndex_(-1)
  , networkDetectionManager_(ndManager)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);
    connect(networkDetectionManager_, SIGNAL(primaryAdapterUp(ProtoTypes::NetworkInterface)), SLOT(onPrimaryAdapterUp(ProtoTypes::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterDown(ProtoTypes::NetworkInterface)), SLOT(onPrimaryAdapterDown(ProtoTypes::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterChanged(ProtoTypes::NetworkInterface)), SLOT(onPrimaryAdapterChanged(ProtoTypes::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(primaryAdapterNetworkLostOrChanged(ProtoTypes::NetworkInterface)), SLOT(onPrimaryAdapterNetworkLostOrChanged(ProtoTypes::NetworkInterface)));
    connect(networkDetectionManager_, SIGNAL(networkListChanged(ProtoTypes::NetworkInterfaces)), SLOT(onNetworkListChanged(ProtoTypes::NetworkInterfaces)));
    connect(networkDetectionManager_, SIGNAL(wifiAdapterChanged(bool)), SLOT(onWifiAdapterChaged(bool)));

    lastSpoofTime_ = QDateTime::currentDateTime();
}

MacAddressController_mac::~MacAddressController_mac()
{

}

void MacAddressController_mac::initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(macAddrSpoofing, macAddrSpoofing_))
    {
        if (macAddrSpoofing.is_enabled())
        {
            autoRotate_ = macAddrSpoofing.is_auto_rotate();
        }

        macAddrSpoofing_ = macAddrSpoofing;
    }
}

void MacAddressController_mac::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(macAddrSpoofing, macAddrSpoofing_))
    {
        qCDebug(LOG_BASIC) << "MacAddrSpoofing has changed";
        qCDebug(LOG_BASIC) << QString::fromStdString(macAddrSpoofing.DebugString());

        ProtoTypes::NetworkInterface currentAdapter = MacUtils::currentNetworkInterface();
        ProtoTypes::NetworkInterface selectedInterface = macAddrSpoofing.selected_network_interface();

        if (macAddrSpoofing.is_enabled())
        {
            QMap<QString, QString> spoofsToApply;
            QMap<QString, QString> spoofsToRemove;

            if (actuallyAutoRotate_) // auto-rotate
            {
                const ProtoTypes::NetworkInterface lastInterface = networkDetectionManager_->lastNetworkInterface();
                int spoofIndex = lastInterface.interface_index();
                if (spoofIndex != -1)
                {
                    lastSpoofIndex_ = spoofIndex;
                    qCDebug(LOG_BASIC) << "Auto-rotating spoof on interface: " << spoofIndex;
                    const QString macAddress(QString::fromStdString(macAddrSpoofing.mac_address()));

                    const QString lastInterfaceName = QString::fromStdString(lastInterface.interface_name());
                    if (MacUtils::isAdapterUp(QString::fromStdString(lastInterface.interface_name()))) // apply-able interface
                    {
                        spoofsToApply.insert(lastInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                    }
                    else
                    {
                        qCDebug(LOG_BASIC) << "Can't auto-rotate - adapter not up";
                    }

                    // remove all but last
                    const ProtoTypes::NetworkInterfaces spoofedInterfacesExceptLast = Utils::interfacesExceptOne(MacUtils::currentSpoofedInterfaces(), lastInterface);
                    for (const ProtoTypes::NetworkInterface &networkInterface : spoofedInterfacesExceptLast.networks())
                    {
                        spoofsToRemove.insert(QString::fromStdString(networkInterface.interface_name()), "");
                    }

                    actuallyAutoRotate_ = false;
                }
                else
                {
                    qCDebug(LOG_BASIC) << "Cannot Auto-rotate on No Interface";
                }
            }
            else // Manual press or Update
            {
                // apply spoof to current adapter if same as selected
                if (!macAddrSpoofing_.is_enabled() ||
                    macAddrSpoofing.mac_address() != macAddrSpoofing_.mac_address() ||
                    selectedInterface.interface_index() != macAddrSpoofing_.selected_network_interface().interface_index())
                {
                    // apply spoof to current adapter
                    // changed enable state, mac address or interface
                    if (selectedInterface.interface_index() == currentAdapter.interface_index())
                    {
                        // valid adapter
                        if (currentAdapter.interface_index() != Utils::noNetworkInterface().interface_index())
                        {
                            int spoofIndex = currentAdapter.interface_index();
                            lastSpoofIndex_ = spoofIndex;
                            QString macAddress(QString::fromStdString(macAddrSpoofing.mac_address()));
                            qCDebug(LOG_BASIC) << "Manual spoof on interface: " << spoofIndex;

                            const QString currentInterfaceName = QString::fromStdString(currentAdapter.interface_name());
                            if (MacUtils::isAdapterUp(QString::fromStdString(currentAdapter.interface_name()))) // apply-able interface
                            {
                                spoofsToApply.insert(currentInterfaceName, macAddress); // latest address will be spoofed in the case of duplicates
                            }
                            else
                            {
                                qCDebug(LOG_BASIC) << "Adapter not up -- cannot apply spoof";
                            }

                            // remove all spoofs except current
                            const ProtoTypes::NetworkInterfaces spoofedInterfacesExceptLast = Utils::interfacesExceptOne(MacUtils::currentSpoofedInterfaces(), currentAdapter);
                            for (const ProtoTypes::NetworkInterface &networkInterface : spoofedInterfacesExceptLast.networks())
                            {
                                spoofsToRemove.insert(QString::fromStdString(networkInterface.interface_name()), "");
                            }
                        }
                        else
                        {
                            qCDebug(LOG_BASIC) << "Can't spoof No Interface";
                        }
                    }
                    else
                    {
                        qCDebug(LOG_BASIC) << "Selected adapter is different than Current";
                    }
                }
                else
                {
                    qCDebug(LOG_BASIC) << "Non-triggering MacAddrSpoofing change";
                }
            }

            networksBeingUpdated_.clear();
            applyAndRemoveSpoofs(spoofsToApply, spoofsToRemove);
        }
        else
        {
            qCDebug(LOG_BASIC) << "MAC spoofing is disabled";

            QMap<QString, QString> spoofsToRemove;
            const ProtoTypes::NetworkInterfaces spoofs = MacUtils::currentSpoofedInterfaces();
            for (const ProtoTypes::NetworkInterface &networkInterface : spoofs.networks())
            {
                spoofsToRemove.insert(QString::fromStdString(networkInterface.interface_name()), "");
            }

            networksBeingUpdated_.clear();
            removeSpoofs(spoofsToRemove);
        }

        // update state
        autoRotate_ = macAddrSpoofing.is_auto_rotate();
        macAddrSpoofing_ = macAddrSpoofing;
    }
}

void MacAddressController_mac::updateNetworkList()
{
    ProtoTypes::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofingWithUpdatedNetworkList();
    setMacAddrSpoofing(updatedMacAddrSpoofing);
    emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
}

void MacAddressController_mac::onPrimaryAdapterUp(const ProtoTypes::NetworkInterface &currentAdapter)
{
    if (doneFilteringNetworkEvents())
    {
        qCDebug(LOG_BASIC) << "Finished filtering network events";
    }

    qCDebug(LOG_BASIC) << "Set MAC spoofing (Up)";
    updateNetworkList();
}

void MacAddressController_mac::onPrimaryAdapterDown(const ProtoTypes::NetworkInterface &lastAdapter)
{
    Q_UNUSED(lastAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onPrimaryAdapterChanged(const ProtoTypes::NetworkInterface &currentAdapter)
{
    Q_UNUSED(currentAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onPrimaryAdapterNetworkLostOrChanged(const ProtoTypes::NetworkInterface &currentAdapter)
{
    Q_UNUSED(currentAdapter);
    filterAndRotateOrUpdateList();
}

void MacAddressController_mac::onNetworkListChanged(const ProtoTypes::NetworkInterfaces &adapterList)
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
    ProtoTypes::NetworkInterfaces networkInterfaces = MacUtils::currentNetworkInterfaces(true);
    for (const ProtoTypes::NetworkInterface &networkInterface : networkInterfaces.networks())
    {
        if (networksBeingUpdated_.contains(QString::fromStdString(networkInterface.interface_name())))
        {
            if (MacUtils::isAdapterUp(QString::fromStdString(networkInterface.interface_name())))
            {
                networksBeingUpdated_.removeAll(QString::fromStdString(networkInterface.interface_name()));
            }
        }
    }

    return networksBeingUpdated_.size() == 0;
}

void MacAddressController_mac::autoRotateUpdateMacSpoof()
{
    ProtoTypes::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofingWithUpdatedNetworkList();

    bool lastIsSameAsSelected = networkDetectionManager_->lastNetworkInterface().interface_index() == updatedMacAddrSpoofing.selected_network_interface().interface_index();
    bool lastUp = MacUtils::isAdapterUp(QString::fromStdString(networkDetectionManager_->lastNetworkInterface().interface_name()));

    if (autoRotate_ && lastIsSameAsSelected && lastUp) // apply-able interface
    {
        qCDebug(LOG_BASIC) << "Auto-rotate generating MAC";
        QString newMacAddress = Utils::generateRandomMacAddress();
        updatedMacAddrSpoofing.set_mac_address(newMacAddress.toStdString());
        actuallyAutoRotate_ = true;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Couldn't rotate";
        qCDebug(LOG_BASIC) << "Auto rotate ON: " << autoRotate_;
        qCDebug(LOG_BASIC) << "Last is same as selected: " << lastIsSameAsSelected << " "
                           << QString::fromStdString(networkDetectionManager_->lastNetworkInterface().interface_name()) << " (last) "
                           << QString::fromStdString(updatedMacAddrSpoofing.selected_network_interface().interface_name()) << "(selected)";
        qCDebug(LOG_BASIC) << "Last is up?: " << lastUp;
    }

    qCDebug(LOG_BASIC) << "Setting MAC Spoofing (Auto)";
    setMacAddrSpoofing(updatedMacAddrSpoofing);
    emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
}

ProtoTypes::MacAddrSpoofing MacAddressController_mac::macAddrSpoofingWithUpdatedNetworkList()
{
    ProtoTypes::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
    ProtoTypes::NetworkInterfaces networkInterfaces = MacUtils::currentNetworkInterfaces(true);

    // update adapter list
    *updatedMacAddrSpoofing.mutable_network_interfaces() = networkInterfaces;

    // update selection
    bool found = false;
    for (const ProtoTypes::NetworkInterface &someInterface : networkInterfaces.networks())
    {
        if (updatedMacAddrSpoofing.selected_network_interface().interface_index() != someInterface.interface_index())
        {
            found = true;
        }
    }

    if (!found)
    {
        *updatedMacAddrSpoofing.mutable_selected_network_interface() = Utils::noNetworkInterface();
    }

    return updatedMacAddrSpoofing;
}

void MacAddressController_mac::applyMacAddressSpoof(const QString &interfaceName, const QString &macAddress)
{
    QString command = "ifconfig " + interfaceName + " ether " + macAddress + " 2> /dev/null && echo success";
    QString result = helper_->executeRootCommand(command);

    // ifconfig command could error or simply fail to update the MAC address
    if (result.contains("success")) // detect ifconfig error
    {
        // can verify MAC spoof immediately on Mac (no need to wait for adapter reset like on Windows)
        if (!MacUtils::interfaceSpoofed(interfaceName) || !MacUtils::checkMacAddr(interfaceName, macAddress)) // detect failure to update MAC address
        {
            // Try another robust command.
            command = "/System/Library/PrivateFrameworks/Apple80211.framework/Resources/airport -z; ifconfig "
                + interfaceName + " ether " + macAddress + "; ifconfig " + interfaceName + " up;";
            result = helper_->executeRootCommand(command);

            if (!MacUtils::interfaceSpoofed(interfaceName) || !MacUtils::checkMacAddr(interfaceName, macAddress)) // detect failure to update MAC address
            {
                qCDebug(LOG_BASIC) << "Could not spoof MAC address, try updating your OS to the latest version. : " <<  interfaceName;
                helper_->enableMacSpoofingOnBoot(false, "", "");
                emit sendUserWarning(ProtoTypes::USER_WARNING_MAC_SPOOFING_FAILURE_SOFT);
                return;
            }
        }

        // apply to boot script if successful
        lastSpoofTime_ = QDateTime::currentDateTime();
        qCDebug(LOG_BASIC) << "Successfully updated MAC address: " << interfaceName << " : " << macAddress;
        helper_->enableMacSpoofingOnBoot(true, interfaceName, macAddress);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Ifconfig failed on: " << interfaceName;
        emit sendUserWarning(ProtoTypes::USER_WARNING_MAC_SPOOFING_FAILURE_HARD);
    }
}

void MacAddressController_mac::removeMacAddressSpoof(const QString &interfaceName)
{
    helper_->enableMacSpoofingOnBoot(false, "", "");
    QString originalMacAddress = MacUtils::trueMacAddress(interfaceName);
    QString commandRemoveSpoofedMacAddress = "ifconfig " + interfaceName + " ether " + originalMacAddress;
    helper_->executeRootCommand(commandRemoveSpoofedMacAddress);
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
