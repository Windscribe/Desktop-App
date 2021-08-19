#include "networkdetectionmanager_win.h"

#include "utils/winutils.h"
#include "utils/logger.h"
#include "utils/utils.h"


const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

NetworkDetectionManager_win::NetworkDetectionManager_win(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
{
    helper_ = dynamic_cast<Helper_win *>(helper);
    Q_ASSERT(helper_);
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
    ProtoTypes::NetworkInterface ni = WinUtils::interfaceByIndex(interfaceIndex, enabled);
    return enabled;
}

void NetworkDetectionManager_win::applyMacAddressSpoof(int ifIndex, QString macAddress)
{
    QString interfaceSubkeyN = WinUtils::interfaceSubkeyName(ifIndex);

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
    QString interfaceSubkeyN = WinUtils::interfaceSubkeyName(ifIndex);

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
    QString subkeyName = WinUtils::interfaceSubkeyName(ifIndex);

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
    updateCurrentNetworkInterface();
}

void NetworkDetectionManager_win::updateCurrentNetworkInterface(bool requested)
{
    ProtoTypes::NetworkInterface curNetworkInterface = WinUtils::currentNetworkInterface();
    curNetworkInterface.set_requested(requested);

    // Only report a changed, properly formed
    if (lastSentNetworkInterface_.active() != curNetworkInterface.active()
        || !Utils::sameNetworkInterface(lastSentNetworkInterface_, curNetworkInterface))
    {
        const QString name = QString::fromStdString(curNetworkInterface.network_or_ssid());
        if (name != "Unidentified network" && name != "Identifying...")
        {
            // if still online: Don't send the NoInterface since this can happen during
            // VPN CONNECTING
            if (curNetworkInterface.interface_index() == -1)
            {
                if (!isOnline())
                {
                    lastSentNetworkInterface_ = curNetworkInterface;
                    emit networkChanged(curNetworkInterface);
                }
            }
            else
            {
                lastSentNetworkInterface_ = curNetworkInterface;
                emit networkChanged(curNetworkInterface);
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Network update skipped: unidentified network (interface ="
                << curNetworkInterface.friendly_name().c_str() << "id ="
                << curNetworkInterface.interface_index() << " active ="
                << curNetworkInterface.active() << ")";
        }
    }
}

bool NetworkDetectionManager_win::isOnline()
{
    bool result = false;

    ProtoTypes::NetworkInterfaces nis = WinUtils::currentNetworkInterfaces(true);

    for (int i = 0; i < nis.networks_size(); i++)
    {
        ProtoTypes::NetworkInterface ni = nis.networks(i);

        if (ni.active())
        {
            result = true;
            break;
        }
    }

    return result;
}
