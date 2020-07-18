#include "networkdetectionmanager_mac.h"

#include <google/protobuf/util/message_differencer.h>
#include <QRegularExpression>
#include "../networkstatemanager/reachabilityevents.h"
#include "utils/macutils.h"
#include "utils/utils.h"
#include "utils/logger.h"

const int typeIdNetworkInterface = qRegisterMetaType<ProtoTypes::NetworkInterface>("ProtoTypes::NetworkInterface");

NetworkDetectionManager_mac::NetworkDetectionManager_mac(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
    , helper_(helper)
    , lastWifiAdapterUp_(false)
{
    connect(&ReachAbilityEvents::instance(), SIGNAL(networkStateChanged()), SLOT(onNetworkStateChanged()));
    lastNetworkInterface_ = MacUtils::currentNetworkInterface();
    lastNetworkList_ = MacUtils::currentNetworkInterfaces(true);
    lastWifiAdapterUp_ = MacUtils::currentlyUpWifiInterfaces().networks_size() > 0;
}

NetworkDetectionManager_mac::~NetworkDetectionManager_mac()
{
}

bool NetworkDetectionManager_mac::isOnline()
{
    QMutexLocker locker(&mutex_);
    QString strNetworkInterface;
    bool b = checkOnline(strNetworkInterface);
    return b;
}

const ProtoTypes::NetworkInterface NetworkDetectionManager_mac::lastNetworkInterface()
{
    return lastNetworkInterface_;
}

void NetworkDetectionManager_mac::updateCurrentNetworkInterface(bool requested)
{
    QMutexLocker locker(&mutex_);

    ProtoTypes::NetworkInterface networkInterface = MacUtils::currentNetworkInterface();
    networkInterface.set_requested(requested);
    lastNetworkInterface_ = networkInterface;

    emit networkChanged(networkInterface);
}

void NetworkDetectionManager_mac::onNetworkStateChanged()
{
    const ProtoTypes::NetworkInterface &networkInterface = MacUtils::currentNetworkInterface();
    const ProtoTypes::NetworkInterfaces &networkList = MacUtils::currentNetworkInterfaces(true);

    const ProtoTypes::NetworkInterfaces &wifiInterfaces = MacUtils::currentlyUpWifiInterfaces();
    bool wifiAdapterUp = wifiInterfaces.networks_size() > 0;

    if (!google::protobuf::util::MessageDifferencer::Equals(networkInterface, lastNetworkInterface_))
    {
        if (networkInterface.interface_name() != lastNetworkInterface_.interface_name())
        {
            if (networkInterface.interface_index() == -1)
            {
                qCDebug(LOG_BASIC) << "Primary Adapter down: " << QString::fromStdString(lastNetworkInterface_.interface_name());
                emit primaryAdapterDown(lastNetworkInterface_);
            }
            else if (lastNetworkInterface_.interface_index() == -1)
            {
                qCDebug(LOG_BASIC) << "Primary Adapter up: " << QString::fromStdString(networkInterface.interface_name());
                emit primaryAdapterUp(networkInterface);
            }
            else
            {
                qCDebug(LOG_BASIC) << "Primary Adapter changed: " << QString::fromStdString(networkInterface.interface_name());
                emit primaryAdapterChanged(networkInterface);
            }
        }
        else if (networkInterface.network_or_ssid() != lastNetworkInterface_.network_or_ssid())
        {
            qCDebug(LOG_BASIC) << "Primary Network Changed: "
                               << QString::fromStdString(networkInterface.interface_name())
                               << " : " << QString::fromStdString(networkInterface.network_or_ssid());

            // if network change or adapter down (no comeup)
            if (lastNetworkInterface_.network_or_ssid() != "")
            {
                qCDebug(LOG_BASIC) << "Primary Adapter Network changed or lost";
                emit primaryAdapterNetworkLostOrChanged(networkInterface);
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Unidentified interface change";
            // Can happen when changing interfaces
        }

        lastNetworkInterface_ = networkInterface;
        emit networkChanged(networkInterface);
    }
    else if (wifiAdapterUp != lastWifiAdapterUp_)
    {
        if (MacUtils::isWifiAdapter(QString::fromStdString(networkInterface.interface_name()))
                || MacUtils::isWifiAdapter(QString::fromStdString(lastNetworkInterface_.interface_name())))
        {
            qCDebug(LOG_BASIC) << "Wifi adapter (primary) up state changed: " << wifiAdapterUp;
            emit wifiAdapterChanged(wifiAdapterUp);
        }
        emit networkChanged(networkInterface);
    }
    else if (!google::protobuf::util::MessageDifferencer::Equals(networkList, lastNetworkList_))
    {
        qCDebug(LOG_BASIC) << "Network list changed";
        emit networkListChanged(networkList);
        emit networkChanged(networkInterface);
    }

    lastNetworkList_ = networkList;
    lastWifiAdapterUp_ = wifiAdapterUp;
}

bool NetworkDetectionManager_mac::checkOnline(QString &networkInterface)
{
    QString strReply;
#ifdef Q_OS_MAC

    FILE *file = popen("echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryInterface | sed -e 's/.*PrimaryInterface : //'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    strReply = strReply.trimmed();
    networkInterface = strReply;
#endif
    if (strReply.isEmpty())
    {
        return false;
    }
    else
    {
        return true;
    }
}

