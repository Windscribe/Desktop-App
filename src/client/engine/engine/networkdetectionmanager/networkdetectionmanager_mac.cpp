#include "networkdetectionmanager_mac.h"

#include <QRegularExpression>

#include "../networkdetectionmanager/reachabilityevents.h"
#include "utils/interfaceutils_mac.h"
#include "utils/log/categories.h"
#include "utils/macutils.h"
#include "utils/network_utils/network_utils.h"
#include "utils/utils.h"

const int typeIdNetworkInterface = qRegisterMetaType<types::NetworkInterface>("types::NetworkInterface");

NetworkDetectionManager_mac::NetworkDetectionManager_mac(QObject *parent, Helper *helper) : INetworkDetectionManager (parent)
{
    connect(&ReachAbilityEvents::instance(), &ReachAbilityEvents::networkStateChanged, this, &NetworkDetectionManager_mac::onNetworkStateChanged);
    connect(&pathMonitor_, &PathMonitor::connectivityStateChanged, this, &NetworkDetectionManager_mac::onOnlineStateChanged);
    lastNetworkInterface_ = currentNetworkInterfaceFromNetworkList(InterfaceUtils_mac::instance().currentNetworkInterfaces(true));
    lastIsOnlineState_ = pathMonitor_.isOnline();
}

NetworkDetectionManager_mac::~NetworkDetectionManager_mac()
{
}

bool NetworkDetectionManager_mac::isOnline()
{
    return lastIsOnlineState_;
}

void NetworkDetectionManager_mac::onNetworkStateChanged()
{
    onOnlineStateChanged(pathMonitor_.isOnline());

    const QVector<types::NetworkInterface> &networkList = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);
    const types::NetworkInterface &networkInterface = currentNetworkInterfaceFromNetworkList(networkList);

    if (networkInterface != lastNetworkInterface_)
    {
        bool significantChange = false;

        if (networkInterface.interfaceName != lastNetworkInterface_.interfaceName)
        {
            significantChange = true;
            if (networkInterface.interfaceIndex == -1)
            {
                qCInfo(LOG_BASIC) << "Primary Adapter down: " << lastNetworkInterface_.interfaceName;
            }
            else if (lastNetworkInterface_.interfaceIndex == -1)
            {
                qCInfo(LOG_BASIC) << "Primary Adapter up: " << networkInterface.interfaceName;
            }
            else
            {
                qCInfo(LOG_BASIC) << "Primary Adapter changed: " << networkInterface.interfaceName;
            }
        }
        else if (networkInterface.networkOrSsid != lastNetworkInterface_.networkOrSsid)
        {
            significantChange = true;
            qCInfo(LOG_BASIC) << "Primary Network Changed: "
                               << networkInterface.interfaceName
                               << " : " << networkInterface.networkOrSsid;
        }
        else
        {
            qCInfo(LOG_BASIC) << "Minor interface change on" << networkInterface.interfaceName
                              << "(e.g. same-SSID AP roam), skipping reconnect";
        }

        lastNetworkInterface_ = networkInterface;
        if (significantChange) {
            emit networkChanged(networkInterface);
        }
    }
}

void NetworkDetectionManager_mac::onOnlineStateChanged(bool isOnline)
{
    if (lastIsOnlineState_ != isOnline) {
        lastIsOnlineState_ = isOnline;
        emit onlineStateChanged(isOnline);
    }
}

const types::NetworkInterface NetworkDetectionManager_mac::currentNetworkInterfaceFromNetworkList(const QVector<types::NetworkInterface> &networkList)
{
    // we assume that the first non-empty adapter is a current network interface
    for (int i = 0; i < networkList.size(); ++i)
    {
        if (networkList[i].interfaceIndex != -1)
        {
            return networkList[i];
        }
    }

    return types::NetworkInterface::noNetworkInterface();
}

void NetworkDetectionManager_mac::getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate)
{
    if (forceUpdate) {
        onNetworkStateChanged();
    }
    networkInterface = lastNetworkInterface_;
}
