#include "networkdetectionmanager_win.h"

#include "utils/log/categories.h"
#include "utils/network_utils/network_utils_win.h"
#include "utils/ws_assert.h"

NetworkDetectionManager_win::NetworkDetectionManager_win(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    WS_ASSERT(helper_);

    curNetworkInterface_ = NetworkUtils_win::currentNetworkInterface();
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
    QString subkeyName = NetworkUtils_win::interfaceSubkeyName(ifIndex);

    if (subkeyName != "")
    {
        helper_->resetNetworkAdapter(subkeyName, bringBackUp);
    }
    else
    {
        qCWarning(LOG_BASIC) << "Couldn't reset adapter -- No index matching adapter in Registry:" << ifIndex;
    }
}

void NetworkDetectionManager_win::onNetworkChanged()
{
    bool bCurIsOnline = isOnlineImpl();
    if (bLastIsOnline_ != bCurIsOnline) {
        bLastIsOnline_ = bCurIsOnline;
        emit onlineStateChanged(bLastIsOnline_);
    }

    // This is a method to check if the current interface has changed, without updating the list of interfaces
    // Doing this avoids e.g. repopulating SSIDs, which causes a location request in Windows 11 24H2 and later.
    QString guid = NetworkUtils_win::currentNetworkInterfaceGuid();
    if (curNetworkInterface_.active && guid == curNetworkInterface_.interfaceGuid) {
        return;
    }

    // Now that we know the interface changed, force an update of the current network interfaces list
    NetworkUtils_win::currentNetworkInterfaces(false, true);

    curNetworkInterface_ = NetworkUtils_win::currentNetworkInterface();

    // If still online, but current interface is "no interface", don't emit the signal
    // In theory this should never happen, since we exclude Windscribe interfaces when getting the current interface,
    // but this code has historically been here and doesn't seem to cause any problems.
    if ((curNetworkInterface_.interfaceIndex == -1 && !bLastIsOnline_) || curNetworkInterface_.interfaceIndex != -1) {
        emit networkChanged(curNetworkInterface_);
    }
}

bool NetworkDetectionManager_win::isOnlineImpl()
{
    return NetworkUtils_win::haveActiveInterface();
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
