#include "macaddresscontroller_win.h"

#include <google/protobuf/util/message_differencer.h>
#include "Utils/utils.h"
#include "Utils/winutils.h"
#include "Utils/logger.h"

const int typeIdMacAddrSpoofing = qRegisterMetaType<ProtoTypes::MacAddrSpoofing>("ProtoTypes::MacAddrSpoofing");
const int typeIdUserWarningType = qRegisterMetaType<ProtoTypes::Protocol>("ProtoTypes::UserWarningType");

MacAddressController_win::MacAddressController_win(QObject *parent, NetworkDetectionManager_win *ndManager) : IMacAddressController (parent)
  , autoRotate_(false)
  , actuallyAutoRotate_(false)
  , lastSpoofIndex_(-1)
  , networkDetectionManager_(ndManager)
{
    connect(networkDetectionManager_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onNetworkChange(ProtoTypes::NetworkInterface)));
}

MacAddressController_win::~MacAddressController_win()
{

}

void MacAddressController_win::initMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(macAddrSpoofing, macAddrSpoofing_))
    {
        if (macAddrSpoofing.is_enabled())
        {
            autoRotate_ = macAddrSpoofing.is_auto_rotate();
        }

        macAddrSpoofing_ = macAddrSpoofing;
        lastNetworkInterface_ = Utils::currentNetworkInterface();
    }
}

void MacAddressController_win::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    // TODO: refactor this function -- see MacAddresController_mac for clean implementation
    if (!google::protobuf::util::MessageDifferencer::Equals(macAddrSpoofing, macAddrSpoofing_))
    {
        QList<int> interfacesToReset;
        ProtoTypes::NetworkInterface currentAdapter = WinUtils::currentNetworkInterface();
        ProtoTypes::NetworkInterface selectedInterface = macAddrSpoofing.selected_network_interface();

        // remove all non-current adapter and current, non-selected adapter spoofs

        ProtoTypes::NetworkInterfaces networkInterfaces = Utils::currentNetworkInterfaces(true);
        for (int i = 0; i < networkInterfaces.networks_size(); i++)
        {
            ProtoTypes::NetworkInterface networkInterface = networkInterfaces.networks(i);
            if (networkInterface.interface_index() != Utils::noNetworkInterface().interface_index())
            {
                bool remove = false;
                if (WinUtils::interfaceSubkeyHasProperty(networkInterface.interface_index(), "WindscribeMACSpoofed")) // has spoof
                {
                    if (macAddrSpoofing.is_enabled())
                    {
                        if (networkInterface.interface_index() == currentAdapter.interface_index()) // current adapter
                        {
                            if (networkInterface.interface_index() != selectedInterface.interface_index()) // non-selected
                            {
                                remove = true;
                            }
                        }
                        else if (networkInterface.interface_index() == lastNetworkInterface_.interface_index()) // last current adapter
                        {
                            if (actuallyAutoRotate_)
                            {
                                int spoofIndex = lastNetworkInterface_.interface_index();
                                lastSpoofIndex_ = spoofIndex;
                                qCDebug(LOG_BASIC) << "Attempting Auto-rotating spoof on interface: " << spoofIndex;
                                QString macAddress(QString::fromStdString(macAddrSpoofing.mac_address()));
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
                        else if (networkInterface.interface_index() == macAddrSpoofing.selected_network_interface().interface_index())
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
                    networkDetectionManager_->removeMacAddressSpoof(networkInterface.interface_index());
                    if (networkDetectionManager_->interfaceEnabled(networkInterface.interface_index())  && !interfacesToReset.contains(networkInterface.interface_index()))
                    {
                        interfacesToReset.append(networkInterface.interface_index());
                    }
                }
            }
        }

        // apply spoof to current adapter if same as selected
        if (selectedInterface.interface_index() == currentAdapter.interface_index())
        {
            // apply spoof to current adapter
            if (macAddrSpoofing.is_enabled())
            {
                // changed enable state, mac address or interface
                if (!macAddrSpoofing_.is_enabled() ||
                    macAddrSpoofing.mac_address() != macAddrSpoofing_.mac_address() ||
                    selectedInterface.interface_index() != macAddrSpoofing_.selected_network_interface().interface_index())
                {
                    // valid adapter
                    if (currentAdapter.interface_index() != Utils::noNetworkInterface().interface_index())
                    {
                        int spoofIndex = currentAdapter.interface_index();
                        lastSpoofIndex_ = spoofIndex;
                        QString macAddress(QString::fromStdString(macAddrSpoofing.mac_address()));
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
        foreach (int indexToReset, interfacesToReset)
        {
            networksBeingUpdated_.append(indexToReset);
            networkDetectionManager_->resetAdapter(indexToReset);
        }

        // update state
        autoRotate_ = macAddrSpoofing.is_auto_rotate();
        macAddrSpoofing_ = macAddrSpoofing;
        lastNetworkInterface_ = currentAdapter;
    }
}

void MacAddressController_win::onNetworkChange(ProtoTypes::NetworkInterface networkInterface)
{
    Q_UNUSED(networkInterface);

    // filter network events when adapters are being reset
    ProtoTypes::NetworkInterfaces networkInterfaces = Utils::currentNetworkInterfaces(true);
    for (int i = 0; i < networkInterfaces.networks_size(); i++)
    {
        ProtoTypes::NetworkInterface networkInterface = networkInterfaces.networks(i);

        if (networksBeingUpdated_.contains(networkInterface.interface_index()))
        {
            if (networkInterface.active())
            {
                networksBeingUpdated_.removeAll(networkInterface.interface_index());

                // MAC spoof must be checked after the reset on Windows
                if (macAddrSpoofing_.is_enabled())
                {
                    if (networkInterface.interface_index() == lastSpoofIndex_ &&
                        lastSpoofIndex_ != Utils::noNetworkInterface().interface_index() &&
                        networkInterface.interface_index() == macAddrSpoofing_.selected_network_interface().interface_index())
                    {
                        checkMacSpoofAppliedCorrectly();
                    }
                }
            }
        }
    }

    if (networksBeingUpdated_.isEmpty()) // done waiting for resets
    {
        bool setSpoofing = false;

        ProtoTypes::MacAddrSpoofing updatedMacAddrSpoofing = macAddrSpoofing_;
        ProtoTypes::NetworkInterface currentNetwork = Utils::currentNetworkInterface();

        if (currentNetwork.interface_index() != lastNetworkInterface_.interface_index())
        {
            qCDebug(LOG_BASIC) << "Network change detected";

            if (!google::protobuf::util::MessageDifferencer::Equals(networkInterfaces, updatedMacAddrSpoofing.network_interfaces()))
            {
                setSpoofing = true;

                QString adapters = "";
                for (int i = 0; i < networkInterfaces.networks_size(); i++)
                {
                    adapters += QString::fromStdString(networkInterfaces.networks(i).interface_name());

                    if (i < networkInterfaces.networks_size() - 1) adapters += ", ";
                }

                if (networkInterfaces.networks_size() > 0)
                {
                    qCDebug(LOG_BASIC) << "Current active adapters: " << adapters;
                }
                else
                {
                    qCDebug(LOG_BASIC) << "No active adapters";
                }

                *updatedMacAddrSpoofing.mutable_network_interfaces() = networkInterfaces;

                // update selection
                bool found = false;
                for (int i = 0; i < networkInterfaces.networks_size(); i++)
                {
                    ProtoTypes::NetworkInterface someNetwork = networkInterfaces.networks(i);
                    if (updatedMacAddrSpoofing.selected_network_interface().interface_index() != someNetwork.interface_index())
                    {
                        found = true;
                    }
                }

                if (!found)
                {
                    *updatedMacAddrSpoofing.mutable_selected_network_interface() = Utils::noNetworkInterface();
                }

            }
            // qCDebug(LOG_BASIC) << "Selected adapter: " << QString::fromStdString(updatedMacAddrSpoofing.selected_network_interface().interface_name());
            // qCDebug(LOG_BASIC) << "Last current adapter: " << QString::fromStdString(lastNetworkInterface_.interface_name());

            // detect disconnect
            if (lastNetworkInterface_.interface_index() != currentNetwork.interface_index()
                && lastNetworkInterface_.interface_index() != Utils::noNetworkInterface().interface_index())
            {
                if (autoRotate_ &&
                    lastNetworkInterface_.interface_index() == updatedMacAddrSpoofing.selected_network_interface().interface_index() &&
                    networkDetectionManager_->interfaceEnabled(lastNetworkInterface_.interface_index()))
                {
                    // qCDebug(LOG_BASIC) << "Detected disconnect";
                    QString newMacAddress = Utils::generateRandomMacAddress();
                    updatedMacAddrSpoofing.set_mac_address(newMacAddress.toStdString());
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

    QString currentMacAddress = QString::fromStdString(Utils::currentNetworkInterface().physical_address());
    QString attemptedSpoof = QString::fromStdString(macAddrSpoofing_.mac_address());

    if (currentMacAddress != attemptedSpoof.toLower())
    {
    	qCDebug(LOG_BASIC) << "Detected failure to change Mac Address";
        emit sendUserWarning(ProtoTypes::USER_WARNING_MAC_SPOOFING_FAILURE_HARD);

    }

}
