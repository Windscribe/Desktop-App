#include "networkdetectionmanager_win.h"

#include <optional>

#include "utils/log/categories.h"
#include "utils/network_utils/network_utils_win.h"
#include "utils/ws_assert.h"

NetworkDetectionManager_win::NetworkDetectionManager_win(QObject *parent, Helper *helper) : INetworkDetectionManager (parent),
    helper_(helper)
{
    curNetworkInterface_ = NetworkUtils_win::currentNetworkInterface();
    curNetworkId_ = NetworkUtils_win::networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    bLastIsOnline_ = isOnlineImpl();

    networkWorker_ = new NetworkChangeWorkerThread(this);

    connect(networkWorker_, &NetworkChangeWorkerThread::finished, networkWorker_, &QObject::deleteLater);
    connect(networkWorker_, &NetworkChangeWorkerThread::networkChanged, this, &NetworkDetectionManager_win::onNetworkChanged, Qt::QueuedConnection);

    networkWorker_->start();
}

NetworkDetectionManager_win::~NetworkDetectionManager_win()
{
    networkWorker_->earlyExit();
    networkWorker_->wait();
}


bool NetworkDetectionManager_win::interfaceEnabled(int interfaceIndex)
{
    bool enabled = false;
    types::NetworkInterface ni = NetworkUtils_win::interfaceByIndex(interfaceIndex, enabled);
    return enabled;
}

void NetworkDetectionManager_win::applyMacAddressSpoof(int ifIndex, QString macAddress)
{
    QString interfaceSubkeyN = NetworkUtils_win::interfaceSubkeyName(ifIndex);

    if (interfaceSubkeyN != "")
    {
        helper_->setMacAddressRegistryValueSz(interfaceSubkeyN, macAddress);
    }
    else
    {
        qCWarning(LOG_BASIC) << "Apply MacAddress Failed. Couldn't find adapter in Registry matching interface " << ifIndex;
    }
}

void NetworkDetectionManager_win::removeMacAddressSpoof(int ifIndex)
{
    qCInfo(LOG_BASIC) << "Removing spoof on interface: " << ifIndex;
    QString interfaceSubkeyN = NetworkUtils_win::interfaceSubkeyName(ifIndex);

    if (interfaceSubkeyN != "")
    {
        helper_->removeMacAddressRegistryProperty(interfaceSubkeyN);
    }
    else
    {
        qCWarning(LOG_BASIC) << "Remove MacAddress failed. Couldn't find adapter in Registry matching interface " << ifIndex;
    }
}

void NetworkDetectionManager_win::resetAdapter(int ifIndex, bool bringBackUp)
{
    qCInfo(LOG_BASIC) << "Resetting interface: " << ifIndex;
    helper_->resetNetworkAdapter(ifIndex, bringBackUp);
}

void NetworkDetectionManager_win::onNetworkChanged()
{
    bool bCurIsOnline = isOnlineImpl();
    if (bLastIsOnline_ != bCurIsOnline) {
        bLastIsOnline_ = bCurIsOnline;
        emit onlineStateChanged(bLastIsOnline_);
    }

    // Check if the current interface or its network changed, without updating the list of interfaces.
    // Doing this avoids e.g. repopulating SSIDs, which causes a location request in Windows 11 24H2 and later.
    // The network id catches joining a different network on the same adapter, e.g. across a sleep/wake.
    QString guid = NetworkUtils_win::currentNetworkInterfaceGuid();
    std::optional<QString> networkId = NetworkUtils_win::networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    bool networkIdChanged;
    if (networkId.has_value()) {
        // No trusted baseline means the network may have changed while the id was unavailable; refresh to resync.
        networkIdChanged = !curNetworkId_.has_value() || *networkId != *curNetworkId_;
        refreshedOnMissingId_ = false;
    } else {
        // A missing id can hide a real change. Refresh once per streak of missing ids: Wi-Fi names come from
        // the WLAN service, so the refresh still gets the right name when the network list has no answer.
        networkIdChanged = !refreshedOnMissingId_;
        refreshedOnMissingId_ = true;
    }
    if (curNetworkInterface_.active && guid == curNetworkInterface_.interfaceGuid && !networkIdChanged) {
        return;
    }

    // Now that we know the interface changed, force an update of the current network interfaces list
    NetworkUtils_win::currentNetworkInterfaces(false, true);

    curNetworkInterface_ = NetworkUtils_win::currentNetworkInterface();
    // Key the id to the interface stored above so the pair can never describe two different adapters.
    curNetworkId_ = NetworkUtils_win::networkIdFromInterfaceGuid(curNetworkInterface_.interfaceGuid);
    // If the id is missing here, this refresh already counts as the missing-id streak's one refresh.
    refreshedOnMissingId_ = !curNetworkId_.has_value();

    // If still online, but current interface is "no interface", don't emit the signal
    // In theory this should never happen, since we exclude app VPN interfaces when getting the current interface,
    // but this code has historically been here and doesn't seem to cause any problems.
    if ((curNetworkInterface_.interfaceIndex == -1 && !bLastIsOnline_) || curNetworkInterface_.interfaceIndex != -1) {
        emit networkChanged(curNetworkInterface_);
    }
}

bool NetworkDetectionManager_win::isOnlineImpl()
{
    return NetworkUtils_win::haveActiveInterface() || NetworkUtils_win::haveInternetConnectivity().value_or(false);
}

void NetworkDetectionManager_win::getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate)
{
    Q_UNUSED(forceUpdate);
    networkInterface = curNetworkInterface_;
}

bool NetworkDetectionManager_win::isOnline()
{
    return bLastIsOnline_;
}
