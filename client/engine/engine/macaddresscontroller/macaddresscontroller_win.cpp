#include "macaddresscontroller_win.h"

#include <optional>

#include "utils/logger.h"
#include "utils/network_utils/network_utils.h"
#include "utils/network_utils/network_utils_win.h"

MacAddressController_win::MacAddressController_win(QObject *parent, NetworkDetectionManager_win *ndManager) : IMacAddressController(parent)
  , networkDetectionManager_(ndManager)
{
    connect(networkDetectionManager_, &NetworkDetectionManager_win::networkChanged, this, &MacAddressController_win::onNetworkChange);
}

MacAddressController_win::~MacAddressController_win()
{
}

void MacAddressController_win::initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_) {
        if (macAddrSpoofing.isEnabled) {
            autoRotate_ = macAddrSpoofing.isAutoRotate;
        }

        macAddrSpoofing_ = macAddrSpoofing;
        lastNetworkInterface_ = NetworkUtils_win::currentNetworkInterface();
    }
}

void MacAddressController_win::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing == macAddrSpoofing_) {
        return;
    }

    qCDebug(LOG_BASIC) << "MacAddressController_win::setMacAddrSpoofing MacAddrSpoofing has changed. actuallyAutoRotate_=" << actuallyAutoRotate_;
    qCDebug(LOG_BASIC) << macAddrSpoofing;

    types::NetworkInterface currentAdapter = NetworkUtils_win::currentNetworkInterface();
    types::NetworkInterface selectedInterface = macAddrSpoofing.selectedNetworkInterface;
    std::optional<int> spoofIndex;
    QList<int> interfacesToReset;

    // Remove all non-current adapter and current, non-selected adapter spoofs.
    const auto networkInterfaces = NetworkUtils_win::currentNetworkInterfaces(false);
    for (const auto &networkInterface : networkInterfaces) {
        bool removeSpoof = false;
        if (NetworkUtils_win::isInterfaceSpoofed(networkInterface.interfaceIndex)) {
            if (macAddrSpoofing.isEnabled) {
                if (networkInterface.interfaceIndex == currentAdapter.interfaceIndex) {
                    if (networkInterface.interfaceIndex != selectedInterface.interfaceIndex) {
                        removeSpoof = true;
                    }
                }
                else if (networkInterface.interfaceIndex == lastNetworkInterface_.interfaceIndex) {
                    if (actuallyAutoRotate_) {
                        // A network change event was detected in onNetworkChange and auto-rotate MAC is enabled.
                        spoofIndex = lastNetworkInterface_.interfaceIndex;
                        actuallyAutoRotate_ = false;
                    }
                }
                else {
                    removeSpoof = true;
                }
            }
            else {
                removeSpoof = true;
            }
        }

        if (removeSpoof) {
            networkDetectionManager_->removeMacAddressSpoof(networkInterface.interfaceIndex);
            if (!interfacesToReset.contains(networkInterface.interfaceIndex) && networkDetectionManager_->interfaceEnabled(networkInterface.interfaceIndex)) {
                interfacesToReset.append(networkInterface.interfaceIndex);
            }
        }
    }

    // Check if we need to apply a manual spoof.
    if (macAddrSpoofing.isEnabled && !spoofIndex.has_value() && selectedInterface.interfaceIndex == currentAdapter.interfaceIndex) {
        // User changed enabled state, mac address or interface
        if (!macAddrSpoofing_.isEnabled || macAddrSpoofing.macAddress != macAddrSpoofing_.macAddress ||
            selectedInterface.interfaceIndex != macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
        {
            if (!types::NetworkInterface::isNoNetworkInterface(currentAdapter.interfaceIndex)) {
                spoofIndex = currentAdapter.interfaceIndex;
            }
        }
    }

    if (spoofIndex.has_value()) {
        lastSpoofIndex_ = spoofIndex.value();
        // Only attempt to apply the spoof if we have a MAC address to spoof with.
        if (!macAddrSpoofing.macAddress.isEmpty()) {
            qCDebug(LOG_BASIC) << "Attempting spoof on interface:" << lastSpoofIndex_;
            networkDetectionManager_->applyMacAddressSpoof(lastSpoofIndex_, macAddrSpoofing.macAddress);
            if (!interfacesToReset.contains(lastSpoofIndex_) && networkDetectionManager_->interfaceEnabled(lastSpoofIndex_)) {
                interfacesToReset.append(lastSpoofIndex_);
            }
        }
    }

    networksBeingUpdated_.clear();

    for (const int indexToReset : interfacesToReset) {
        networksBeingUpdated_.append(indexToReset);
        networkDetectionManager_->resetAdapter(indexToReset);
    }

    autoRotate_ = macAddrSpoofing.isAutoRotate;
    macAddrSpoofing_ = macAddrSpoofing;
    lastNetworkInterface_ = currentAdapter;
}

void MacAddressController_win::onNetworkChange(types::NetworkInterface /*networkInterface*/)
{
    // filter network events when adapters are being reset
    const auto networkInterfaces = NetworkUtils_win::currentNetworkInterfaces(true);
    for (const auto &networkInterface : networkInterfaces) {
        if (networkInterface.active && networksBeingUpdated_.contains(networkInterface.interfaceIndex)) {
            networksBeingUpdated_.removeAll(networkInterface.interfaceIndex);

            // MAC spoof must be checked after the reset on Windows
            if (macAddrSpoofing_.isEnabled) {
                if (networkInterface.interfaceIndex == lastSpoofIndex_ && !types::NetworkInterface::isNoNetworkInterface(lastSpoofIndex_) &&
                    networkInterface.interfaceIndex == macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
                {
                    checkMacSpoofAppliedCorrectly();
                }
            }
        }
    }

    if (!networksBeingUpdated_.isEmpty()) {
        return;
    }

    types::NetworkInterface currentAdapter = NetworkUtils_win::currentNetworkInterface();

    if (currentAdapter.interfaceIndex != lastNetworkInterface_.interfaceIndex) {
        qCDebug(LOG_BASIC) << "MacAddressController network change detected" << currentAdapter.interfaceIndex << lastNetworkInterface_.interfaceIndex << macAddrSpoofing_.selectedNetworkInterface.interfaceIndex;
        types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
        bool setSpoofing = false;

        if (networkInterfaces != updatedMacAddrSpoofing.networkInterfaces) {
            // An adapter has been added/removed or the settings of an existing adapter have
            // changed (e.g. changed wi-fi networks).
            setSpoofing = true;
            qCDebug(LOG_BASIC) << "Current active adapters: " << NetworkUtils::networkInterfacesToString(networkInterfaces, true);

            updatedMacAddrSpoofing.networkInterfaces = networkInterfaces;

            // Check if the spoofed adapter is still in the list.
            bool found = false;
            for (const auto &networkInterface : networkInterfaces) {
                if (updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex == networkInterface.interfaceIndex) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                qCDebug(LOG_BASIC) << "Setting selectedNetworkInterface to 'No Network' interface";
                updatedMacAddrSpoofing.selectedNetworkInterface = types::NetworkInterface::noNetworkInterface();
            }
        }

        // qCDebug(LOG_BASIC) << "Selected adapter: " << QString::fromStdString(updatedMacAddrSpoofing.selected_network_interface().interface_name());
        // qCDebug(LOG_BASIC) << "Last current adapter: " << QString::fromStdString(lastNetworkInterface_.interface_name());

        // detect disconnect
        if (!types::NetworkInterface::isNoNetworkInterface(lastNetworkInterface_.interfaceIndex)) {
            if (autoRotate_ && lastNetworkInterface_.interfaceIndex == updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex &&
                networkDetectionManager_->interfaceEnabled(lastNetworkInterface_.interfaceIndex))
            {
                qCDebug(LOG_BASIC) << "MacAddressController detected disconnect, new MAC address auto-generated";
                updatedMacAddrSpoofing.macAddress = NetworkUtils::generateRandomMacAddress();
                actuallyAutoRotate_ = true;
                setSpoofing = true;
            }
        }

        if (setSpoofing) {
            setMacAddrSpoofing(updatedMacAddrSpoofing);
            emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
        }
    }
}

void MacAddressController_win::checkMacSpoofAppliedCorrectly()
{
    // qCDebug(LOG_BASIC) << "Checking Mac Spoof was applied correctly";

    QString currentMacAddress = NetworkUtils_win::currentNetworkInterface().physicalAddress;
    QString attemptedSpoof = macAddrSpoofing_.macAddress;

    if (currentMacAddress != attemptedSpoof.toLower()) {
        qCDebug(LOG_BASIC) << "Detected failure to change Mac Address";
        emit sendUserWarning(USER_WARNING_MAC_SPOOFING_FAILURE_HARD);
    }
}
