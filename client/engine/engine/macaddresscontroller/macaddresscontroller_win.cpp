#include "macaddresscontroller_win.h"

#include "utils/utils.h"
#include "utils/winutils.h"
#include "utils/logger.h"

MacAddressController_win::MacAddressController_win(QObject *parent, NetworkDetectionManager_win *ndManager) : IMacAddressController (parent)
  , autoRotate_(false)
  , actuallyAutoRotate_(false)
  , lastSpoofIndex_(-1)
  , networkDetectionManager_(ndManager)
{
    connect(networkDetectionManager_, SIGNAL(networkChanged(types::NetworkInterface)), SLOT(onNetworkChange(types::NetworkInterface)));
}

MacAddressController_win::~MacAddressController_win()
{

}

void MacAddressController_win::initMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    if (macAddrSpoofing != macAddrSpoofing_)
    {
        if (macAddrSpoofing.isEnabled)
        {
            autoRotate_ = macAddrSpoofing.isAutoRotate;
        }

        macAddrSpoofing_ = macAddrSpoofing;
        lastNetworkInterface_ = WinUtils::currentNetworkInterface();
    }
}

void MacAddressController_win::setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    // TODO: refactor this function -- see MacAddresController_mac for clean implementation
    if (macAddrSpoofing != macAddrSpoofing_)
    {
        QList<int> interfacesToReset;
        types::NetworkInterface currentAdapter = WinUtils::currentNetworkInterface();
        types::NetworkInterface selectedInterface = macAddrSpoofing.selectedNetworkInterface;

        // remove all non-current adapter and current, non-selected adapter spoofs

        QVector<types::NetworkInterface> networkInterfaces = WinUtils::currentNetworkInterfaces(true);
        for (int i = 0; i < networkInterfaces.size(); i++)
        {
            types::NetworkInterface networkInterface = networkInterfaces[i];
            if (networkInterface.interfaceIndex != Utils::noNetworkInterface().interfaceIndex)
            {
                bool remove = false;
                if (WinUtils::interfaceSubkeyHasProperty(networkInterface.interfaceIndex, "WindscribeMACSpoofed")) // has spoof
                {
                    if (macAddrSpoofing.isEnabled)
                    {
                        if (networkInterface.interfaceIndex == currentAdapter.interfaceIndex) // current adapter
                        {
                            if (networkInterface.interfaceIndex != selectedInterface.interfaceIndex) // non-selected
                            {
                                remove = true;
                            }
                        }
                        else if (networkInterface.interfaceIndex == lastNetworkInterface_.interfaceIndex) // last current adapter
                        {
                            if (actuallyAutoRotate_)
                            {
                                int spoofIndex = lastNetworkInterface_.interfaceIndex;
                                lastSpoofIndex_ = spoofIndex;
                                qCDebug(LOG_BASIC) << "Attempting Auto-rotating spoof on interface: " << spoofIndex;
                                QString macAddress(macAddrSpoofing.macAddress);
                                networkDetectionManager_->applyMacAddressSpoof(spoofIndex, macAddress);
                                if (networkDetectionManager_->interfaceEnabled(spoofIndex)  && !interfacesToReset.contains(spoofIndex))
                                {
                                    interfacesToReset.append(spoofIndex);
                                }
                                actuallyAutoRotate_ = false;
                            }
                            else
                            {
                                // do nothing
                            }
                        }
                        else if (networkInterface.interfaceIndex == macAddrSpoofing.selectedNetworkInterface.interfaceIndex)
                        {
                            // do nothing
                        }
                        else
                        {
                            remove = true;
                        }
                    }
                    else
                    {
                        remove = true;
                    }
                }

                if (remove)
                {
                    networkDetectionManager_->removeMacAddressSpoof(networkInterface.interfaceIndex);
                    if (networkDetectionManager_->interfaceEnabled(networkInterface.interfaceIndex)  && !interfacesToReset.contains(networkInterface.interfaceIndex))
                    {
                        interfacesToReset.append(networkInterface.interfaceIndex);
                    }
                }
            }
        }

        // apply spoof to current adapter if same as selected
        if (selectedInterface.interfaceIndex == currentAdapter.interfaceIndex)
        {
            // apply spoof to current adapter
            if (macAddrSpoofing.isEnabled)
            {
                // changed enable state, mac address or interface
                if (!macAddrSpoofing_.isEnabled ||
                    macAddrSpoofing.macAddress != macAddrSpoofing_.macAddress ||
                    selectedInterface.interfaceIndex != macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
                {
                    // valid adapter
                    if (currentAdapter.interfaceIndex != Utils::noNetworkInterface().interfaceIndex)
                    {
                        int spoofIndex = currentAdapter.interfaceIndex;
                        lastSpoofIndex_ = spoofIndex;
                        QString macAddress(macAddrSpoofing.macAddress);
                        qCDebug(LOG_BASIC) << "Attempting Manual spoof on interface: " << spoofIndex;

                        networkDetectionManager_->applyMacAddressSpoof(spoofIndex, macAddress);
                        if (networkDetectionManager_->interfaceEnabled(spoofIndex)  && !interfacesToReset.contains(spoofIndex))
                        {
                            interfacesToReset.append(spoofIndex);
                        }
                    }
                }
            }
        }

        networksBeingUpdated_.clear();

        // reset adapters
        for (int indexToReset : qAsConst(interfacesToReset))
        {
            networksBeingUpdated_.append(indexToReset);
            networkDetectionManager_->resetAdapter(indexToReset);
        }

        // update state
        autoRotate_ = macAddrSpoofing.isAutoRotate;
        macAddrSpoofing_ = macAddrSpoofing;
        lastNetworkInterface_ = currentAdapter;
    }
}

void MacAddressController_win::onNetworkChange(types::NetworkInterface /*networkInterface*/)
{
    // filter network events when adapters are being reset
    QVector<types::NetworkInterface> networkInterfaces = WinUtils::currentNetworkInterfaces(true);
    for (int i = 0; i < networkInterfaces.size(); i++)
    {
        types::NetworkInterface networkInterface = networkInterfaces[i];

        if (networksBeingUpdated_.contains(networkInterface.interfaceIndex))
        {
            if (networkInterface.active)
            {
                networksBeingUpdated_.removeAll(networkInterface.interfaceIndex);

                // MAC spoof must be checked after the reset on Windows
                if (macAddrSpoofing_.isEnabled)
                {
                    if (networkInterface.interfaceIndex == lastSpoofIndex_ &&
                        lastSpoofIndex_ != Utils::noNetworkInterface().interfaceIndex &&
                        networkInterface.interfaceIndex == macAddrSpoofing_.selectedNetworkInterface.interfaceIndex)
                    {
                        checkMacSpoofAppliedCorrectly();
                    }
                }
            }
        }
    }

    if (networksBeingUpdated_.isEmpty()) // done waiting for resets
    {
        types::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
        types::NetworkInterface currentNetwork = WinUtils::currentNetworkInterface();

        if (currentNetwork.interfaceIndex != lastNetworkInterface_.interfaceIndex)
        {
            qCDebug(LOG_BASIC) << "Network change detected";
            bool setSpoofing = false;

            if (networkInterfaces != updatedMacAddrSpoofing.networkInterfaces)
            {
                setSpoofing = true;

                QString adapters = "";
                for (int i = 0; i < networkInterfaces.size(); i++)
                {
                    adapters += networkInterfaces[i].interfaceName;

                    if (i < networkInterfaces.size() - 1) adapters += ", ";
                }

                if (networkInterfaces.size() > 0)
                {
                    qCDebug(LOG_BASIC) << "Current active adapters: " << adapters;
                }
                else
                {
                    qCDebug(LOG_BASIC) << "No active adapters";
                }

                updatedMacAddrSpoofing.networkInterfaces = networkInterfaces;

                // update selection
                bool found = false;
                for (int i = 0; i < networkInterfaces.size(); i++)
                {
                    types::NetworkInterface someNetwork = networkInterfaces[i];
                    if (updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex != someNetwork.interfaceIndex)
                    {
                        found = true;
                    }
                }

                if (!found)
                {
                    updatedMacAddrSpoofing.selectedNetworkInterface = Utils::noNetworkInterface();
                }

            }
            // qCDebug(LOG_BASIC) << "Selected adapter: " << QString::fromStdString(updatedMacAddrSpoofing.selected_network_interface().interface_name());
            // qCDebug(LOG_BASIC) << "Last current adapter: " << QString::fromStdString(lastNetworkInterface_.interface_name());

            // detect disconnect
            if (lastNetworkInterface_.interfaceIndex != currentNetwork.interfaceIndex
                && lastNetworkInterface_.interfaceIndex != Utils::noNetworkInterface().interfaceIndex)
            {
                if (autoRotate_ &&
                    lastNetworkInterface_.interfaceIndex == updatedMacAddrSpoofing.selectedNetworkInterface.interfaceIndex &&
                    networkDetectionManager_->interfaceEnabled(lastNetworkInterface_.interfaceIndex))
                {
                    // qCDebug(LOG_BASIC) << "Detected disconnect";
                    QString newMacAddress = Utils::generateRandomMacAddress();
                    updatedMacAddrSpoofing.macAddress = newMacAddress;
                    actuallyAutoRotate_ = true;
                    setSpoofing = true;
                }
            }

            if (setSpoofing)
            {
                setMacAddrSpoofing(updatedMacAddrSpoofing);
                emit macAddrSpoofingChanged(updatedMacAddrSpoofing);
            }
        }
    }
}

void MacAddressController_win::checkMacSpoofAppliedCorrectly()
{
    // qCDebug(LOG_BASIC) << "Checking Mac Spoof was applied correctly";

    QString currentMacAddress = WinUtils::currentNetworkInterface().physicalAddress;
    QString attemptedSpoof = macAddrSpoofing_.macAddress;

    if (currentMacAddress != attemptedSpoof.toLower())
    {
    	qCDebug(LOG_BASIC) << "Detected failure to change Mac Address";
        emit sendUserWarning(USER_WARNING_MAC_SPOOFING_FAILURE_HARD);

    }
}
