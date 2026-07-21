#include "dnsconfigurator.h"

#include "engine/connectionmanager/connectors/connectionplatformpolicy.h"
#include "engine/crossplatformobjectfactory.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_MACOS
    #include "restorednsmanager_mac.h"
#endif

DnsConfigurator::DnsConfigurator(QObject *parent, Helper *helper, IConnectionPlatformPolicy *platformPolicy,
                                 ICtrldManager *ctrldManager) : IDnsConfigurator(parent),
    helper_(helper),
    platformPolicy_(platformPolicy ? platformPolicy : new ConnectionPlatformPolicy(helper)),
    ctrldManager_(ctrldManager)
{
    if (!ctrldManager_) {
        ctrldManager_ = CrossPlatformObjectFactory::createCtrldManager(this, helper, ExtraConfig::instance().getLogCtrld());
    } else {
        ctrldManager_->setParent(this);
    }
}

void DnsConfigurator::setConnectedDnsInfo(const types::ConnectedDnsInfo &info)
{
    connectedDnsInfo_ = info;
    // Legacy FORCED only survives in old persisted settings and must behave as AUTO here,
    // otherwise primaryDnsServer() would whitelist the ctrld listen IP with no ctrld running.
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_FORCED) {
        connectedDnsInfo_.type = CONNECTED_DNS_TYPE_AUTO;
    }
}

bool DnsConfigurator::usesDnsProxy() const
{
    return (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CUSTOM || connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CONTROLD) &&
           !connectedDnsInfo_.isCustomIPv4Address();
}

bool DnsConfigurator::prepare()
{
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO) {
        platformPolicy_->disableDohIfNeeded();
    }

    // DNS-leak-protection whitelist for this connection attempt. Reset here so a stale set from a
    // previous attempt can never leak through.
    dnsWhitelistIps_.clear();

    // start ctrld utility
    if (usesDnsProxy()) {
        bool bStarted = false;
        if (connectedDnsInfo_.isSplitDns) {
            bStarted = ctrldManager_->runProcess(connectedDnsInfo_.upStream1, connectedDnsInfo_.upStream2, connectedDnsInfo_.hostnames);
        } else {
            bStarted = ctrldManager_->runProcess(connectedDnsInfo_.upStream1, QString(), QStringList());
        }

        if (!bStarted) {
            qCCritical(LOG_BASIC) << "ctrld start failed";
            return false;
        }

        // Whitelist the ctrld listen IP (the system/VPN resolver) plus its plain-DNS (:53) upstreams
        // so DNS-leak protection lets ctrld reach them. On Windows this is applied via
        // setCustomDnsIps below; on macOS/Linux it is carried to the helper via ConnectStatus.
        dnsWhitelistIps_ << ctrldManager_->listenIp();
        dnsWhitelistIps_ << connectedDnsInfo_.ctrldPlainUpstreamIps();
    }

#ifdef Q_OS_WIN
    // we need to exclude these DNS-addresses from DNS leak protection on Windows
    if (usesDnsProxy()) {
        helper_->setCustomDnsIps(dnsWhitelistIps_);
    } else {
        helper_->setCustomDnsIps(connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO ? QStringList() : tunnelDnsServers(QString()));
    }
#endif

    return true;
}

const QStringList &DnsConfigurator::dnsWhitelistIps() const
{
    return dnsWhitelistIps_;
}

QString DnsConfigurator::primaryDnsServer() const
{
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO) {
        return QString();
    } else if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_LOCAL) {
        return "127.0.0.1";
    } else if (connectedDnsInfo_.isCustomIPv4Address()) {
        return connectedDnsInfo_.upStream1;
    } else {
        return ctrldManager_->listenIp();
    }
}

QStringList DnsConfigurator::tunnelDnsServers(const QString &defaultDns) const
{
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO) {
        return QStringList() << defaultDns;
    }

    QStringList dnsIps;
    // For WG protocol we need to add upStream1 address if it's custom ip. Otherwise on Windows WG may not connect.
    if (usesDnsProxy() && NetworkingValidation::isIp(connectedDnsInfo_.upStream1)) {
        dnsIps << connectedDnsInfo_.upStream1;
    }
    dnsIps << primaryDnsServer();
    return dnsIps;
}

void DnsConfigurator::overrideAdapterDns(AdapterGatewayInfo &adapterInfo) const
{
    // override the DNS if we are using custom or local DNS
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CUSTOM || connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CONTROLD ||
        connectedDnsInfo_.type == CONNECTED_DNS_TYPE_LOCAL) {
        const QString dnsIp = primaryDnsServer();
        adapterInfo.setDnsServers({types::IpAddress(dnsIp.toStdString())});
        qCInfo(LOG_CONNECTION) << (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_LOCAL ? "Local" : "Custom")
                               << "DNS detected, will override with:" << dnsIp;
    }
}

void DnsConfigurator::applyWhileConnected(const AdapterGatewayInfo &vpnAdapter, bool isWireGuard)
{
    // For Windows we should to set the custom dns for the adapter explicitly except WireGuard protocol
#ifdef Q_OS_WIN
    if ((connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CUSTOM || connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CONTROLD) && !isWireGuard) {
        WS_ASSERT(vpnAdapter.dnsServers().count() == 1);
        if (!helper_->setCustomDnsWhileConnected(vpnAdapter.ifIndex(),
                                                 QString::fromStdString(vpnAdapter.dnsServers().first().toString()))) {
            qCCritical(LOG_CONNECTED_DNS) << "Failed to set Custom 'while connected' DNS";
        }
    }
#else
    Q_UNUSED(vpnAdapter);
    Q_UNUSED(isWireGuard);
#endif
}

void DnsConfigurator::stopDnsProxy()
{
    ctrldManager_->killProcess();
}

void DnsConfigurator::restoreSystemDns()
{
#ifdef Q_OS_MACOS
    RestoreDNSManager_mac::restoreState(helper_);
#endif
}
