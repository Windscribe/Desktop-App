#include "networkdetectionmanager_win.h"

#include "utils/logger.h"
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
        qCDebug(LOG_BASIC) << "Apply MacAddress Failed. Couldn't find adapter in Registry matching interface " << ifIndex;
    }
}

void NetworkDetectionManager_win::removeMacAddressSpoof(int ifIndex)
{
    qCDebug(LOG_BASIC) << "Removing spoof on interface: " << ifIndex;
    QString interfaceSubkeyN = NetworkUtils_win::interfaceSubkeyName(ifIndex);

    if (interfaceSubkeyN != "")
    {
        helper_->removeMacAddressRegistryProperty(interfaceSubkeyN);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Remove MacAddress failed. Couldn't find adapter in Registry matching interface " << ifIndex;
    }
}

void NetworkDetectionManager_win::resetAdapter(int ifIndex, bool bringBackUp)
{
    qCDebug(LOG_BASIC) << "Resetting interface: " << ifIndex;
    QString subkeyName = NetworkUtils_win::interfaceSubkeyName(ifIndex);

    if (subkeyName != "")
    {
        helper_->resetNetworkAdapter(subkeyName, bringBackUp);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Couldn't reset adapter -- No index matching adapter in Registry:" << ifIndex;
    }
}


void NetworkDetectionManager_win::onNetworkChanged()
{
    bool bCurIsOnline = isOnlineImpl();
    if (bLastIsOnline_ != bCurIsOnline)
    {
        bLastIsOnline_ = bCurIsOnline;
        emit onlineStateChanged(bLastIsOnline_);
    }

    types::NetworkInterface newNetworkInterface = NetworkUtils_win::currentNetworkInterface();
    newNetworkInterface.requested = false;

    // Only report a changed, properly formed
    if (curNetworkInterface_.active != newNetworkInterface.active || !curNetworkInterface_.sameNetworkInterface(newNetworkInterface))
    {
        const QString name = newNetworkInterface.networkOrSsid;
        if (name != "Unidentified network" && name != "Identifying...")
        {
            // if still online: Don't send the NoInterface since this can happen during
            // VPN CONNECTING
            if (newNetworkInterface.interfaceIndex == -1)
            {
                if (!bLastIsOnline_)
                {
                    curNetworkInterface_ = newNetworkInterface;
                    emit networkChanged(curNetworkInterface_);
                }
            }
            else
            {
                curNetworkInterface_ = newNetworkInterface;
                emit networkChanged(curNetworkInterface_);
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Network update skipped: unidentified network ("
                << "valid =" << newNetworkInterface.isValid()
                << "interface =" << newNetworkInterface.friendlyName
                << "id =" << newNetworkInterface.interfaceIndex
                << "active =" << newNetworkInterface.active << ")";
        }
    }
}

bool NetworkDetectionManager_win::isOnlineImpl()
{
    bool result = false;

    QVector<types::NetworkInterface> nis = NetworkUtils_win::currentNetworkInterfaces(true);

    for (const auto &it : nis) {
        if (it.active) {
            result = true;
            break;
        }
    }

    return result;
}

void NetworkDetectionManager_win::getCurrentNetworkInterface(types::NetworkInterface &networkInterface)
{
    networkInterface = curNetworkInterface_;
}

bool NetworkDetectionManager_win::isOnline()
{
    return bLastIsOnline_;
}
