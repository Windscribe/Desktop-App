#include "networkdetectionmanager_mac.h"

#include <QRegularExpression>
#include <wsnet/WSNet.h>

#include "../networkdetectionmanager/reachabilityevents.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"
#include "utils/network_utils/network_utils_mac.h"
#include "utils/interfaceutils_mac.h"
#include "utils/utils.h"
#include "utils/macutils.h"

const int typeIdNetworkInterface = qRegisterMetaType<types::NetworkInterface>("types::NetworkInterface");

NetworkDetectionManager_mac::NetworkDetectionManager_mac(QObject *parent, IHelper *helper, bool isInitialFirewallAlwaysOn) : INetworkDetectionManager (parent)
    , helper_(helper)
    , lastWifiAdapterUp_(false)
{
    connect(&ReachAbilityEvents::instance(), &ReachAbilityEvents::networkStateChanged, this, &NetworkDetectionManager_mac::onNetworkStateChanged);
    lastNetworkList_ = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);
    lastNetworkInterface_ = currentNetworkInterfaceFromNetworkList(lastNetworkList_);
    lastWifiAdapterUp_ = isWifiAdapterUp(lastNetworkList_);

    // In MacOS 15.x+ add additional connectivity checking with DNS lookup tests
    // In MacOS 15.x+ all traffic is blocked for ~30-40s after OS startup when firewall is always on mode is enabled
    // but standard macOS tools show that connectivity is active.
    // As a workaround we add checking DNS queries at some interval (3 sec). Check only for DNS error "Could not contact DNS servers".
    // This check is active only at program startup and if firewall always on is enabled and until the first successful DNS test.
    bool isUseDnsCheckTimer = false;
    if (MacUtils::isOsVersionAtLeast(15, 0)) {
        isUseDnsCheckTimer = isInitialFirewallAlwaysOn;
        qCDebug(LOG_BASIC) << "NetworkDetectionManager_mac will use additional DNS checks for tests connectivity";
    }

    lastIsOnlineState_ = isOnlineImpl(isUseDnsCheckTimer);
    if (!lastIsOnlineState_ && isUseDnsCheckTimer) {
        connect(&dnsCheckTimer_, &QTimer::timeout, this, &NetworkDetectionManager_mac::onCheckDnsTimer);
        qCDebug(LOG_BASIC) << "The firewall is always on option is enabled and no network connectivity. Start the DNS check timer.";
        dnsCheckTimer_.start(3000);
    }
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
    bool curIsOnlineState = isOnlineImpl(dnsCheckTimer_.isActive());
    if (lastIsOnlineState_ != curIsOnlineState)
    {
        lastIsOnlineState_ = curIsOnlineState;
        emit onlineStateChanged(curIsOnlineState);
    }

    if (dnsCheckTimer_.isActive() && lastIsOnlineState_) {
        dnsCheckTimer_.stop();
        qCDebug(LOG_BASIC) << "Stop the DNS check timer in NetworkDetectionManager_mac::onNetworkStateChanged().";
    }

    const QVector<types::NetworkInterface> &networkList = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);
    const types::NetworkInterface &networkInterface = currentNetworkInterfaceFromNetworkList(networkList);
    bool wifiAdapterUp = isWifiAdapterUp(networkList);

    if (networkInterface != lastNetworkInterface_)
    {
        if (networkInterface.interfaceName != lastNetworkInterface_.interfaceName)
        {
            if (networkInterface.interfaceIndex == -1)
            {
                qCInfo(LOG_BASIC) << "Primary Adapter down: " << lastNetworkInterface_.interfaceName;
                emit primaryAdapterDown(lastNetworkInterface_);
            }
            else if (lastNetworkInterface_.interfaceIndex == -1)
            {
                qCInfo(LOG_BASIC) << "Primary Adapter up: " << networkInterface.interfaceName;
                emit primaryAdapterUp(networkInterface);
            }
            else
            {
                qCInfo(LOG_BASIC) << "Primary Adapter changed: " << networkInterface.interfaceName;
                emit primaryAdapterChanged(networkInterface);
            }
        }
        else if (networkInterface.networkOrSsid != lastNetworkInterface_.networkOrSsid)
        {
            qCInfo(LOG_BASIC) << "Primary Network Changed: "
                               << networkInterface.interfaceName
                               << " : " << networkInterface.networkOrSsid;

            // if network change or adapter down (no comeup)
            if (lastNetworkInterface_.networkOrSsid != "")
            {
                qCInfo(LOG_BASIC) << "Primary Adapter Network changed or lost";
                emit primaryAdapterNetworkLostOrChanged(networkInterface);
            }
        }
        else
        {
            qCInfo(LOG_BASIC) << "Unidentified interface change";
            // Can happen when changing interfaces
        }

        lastNetworkInterface_ = networkInterface;
        emit networkChanged(networkInterface);
    }
    else if (wifiAdapterUp != lastWifiAdapterUp_)
    {
        if (NetworkUtils_mac::isWifiAdapter(networkInterface.interfaceName)
                || NetworkUtils_mac::isWifiAdapter(lastNetworkInterface_.interfaceName))
        {
            qCInfo(LOG_BASIC) << "Wifi adapter (primary) up state changed: " << wifiAdapterUp;
            emit wifiAdapterChanged(wifiAdapterUp);
        }
    }
    else if (networkList != lastNetworkList_)
    {
        qCInfo(LOG_BASIC) << "Network list changed";
        emit networkListChanged(networkList);
    }

    lastNetworkList_ = networkList;
    lastWifiAdapterUp_ = wifiAdapterUp;
}

void NetworkDetectionManager_mac::onCheckDnsTimer()
{
    bool curIsOnlineState = isOnlineImpl(true);
    if (lastIsOnlineState_ != curIsOnlineState)
    {
        qCDebug(LOG_BASIC) << "The DNS check timer result: connectivity is active";
        lastIsOnlineState_ = curIsOnlineState;
        emit onlineStateChanged(curIsOnlineState);
    }
    if (lastIsOnlineState_) {
        dnsCheckTimer_.stop();
        qCDebug(LOG_BASIC) << "Stop the DNS check timer in NetworkDetectionManager_mac::onCheckDnsTimer().";
    }
}

bool NetworkDetectionManager_mac::isWifiAdapterUp(const QVector<types::NetworkInterface> &networkList)
{
    for (int i = 0; i < networkList.size(); ++i)
    {
        if (networkList[i].interfaceType == NETWORK_INTERFACE_WIFI)
        {
            return true;
        }
    }
    return false;
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

bool NetworkDetectionManager_mac::isOnlineImpl(bool withDnsServerCheck)
{
    using namespace wsnet;

    QString command = "netstat -nr -f inet | sed '1,3 d' | awk 'NR==1 { for (i=1; i<=NF; i++) { f[$i] = i  } } NR>1 && $(f[\"Destination\"])==\"default\" { print $(f[\"Gateway\"]), $(f[\"Netif\"]) ; exit }'";
    QString strReply = Utils::execCmd(command).trimmed();
    if (strReply.isEmpty())
        return false;

    if (withDnsServerCheck) {
        // It doesn't really matter what domain to check, we're only interested in the error "Could not contact DNS servers".
        auto res = WSNet::instance()->dnsResolver()->lookupBlocked("windscribe.com");
        if (res->isConnectionRefusedError()) {
            qCDebug(LOG_BASIC) << "NetworkDetectionManager_mac::isOnlineImpl, DNS lookupBlocked error:" << QString::fromStdString(res->errorString());
            return false;
        }
    }
    return true;
}

void NetworkDetectionManager_mac::getCurrentNetworkInterface(types::NetworkInterface &networkInterface, bool forceUpdate)
{
    if (forceUpdate) {
        onNetworkStateChanged();
    }
    networkInterface = lastNetworkInterface_;
}
