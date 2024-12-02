#include "firewallexceptions.h"
#include "uniqueiplist.h"
#include <QThread>
#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "utils/log/categories.h"
#include "engine/dns_utils/dnsutils.h"

void FirewallExceptions::setHostIPs(const QSet<QString> &hostIPs)
{
    hostIPs_ = hostIPs;
}

void FirewallExceptions::setProxyIP(const types::ProxySettings &proxySettings)
{
    if (proxySettings.option() == PROXY_OPTION_NONE) {
        proxyIP_.clear();
    } else if (proxySettings.option() == PROXY_OPTION_HTTP || proxySettings.option() == PROXY_OPTION_SOCKS) {
        proxyIP_ = proxySettings.address();
    } else {
        WS_ASSERT(false);
    }
}

void FirewallExceptions::setCustomRemoteIp(const QString &remoteIP, bool &bChanged)
{
    if (remoteIP_ != remoteIP) {
        remoteIP_ = remoteIP;
        bChanged = true;
    } else {
        bChanged = false;
    }
}

void FirewallExceptions::setConnectingIp(const QString &connectingIp, bool &bChanged)
{
    if (connectingIp_ != connectingIp) {
        connectingIp_ = connectingIp;
        bChanged = true;
    } else {
        bChanged = false;
    }
}

void FirewallExceptions::setDNSServers(const QStringList &ips, bool &bChanged)
{
    if (dnsIps_ != ips) {
        dnsIps_ = ips;
        bChanged = true;
    } else {
        bChanged = false;
    }
}

void FirewallExceptions::setDnsPolicy(DNS_POLICY_TYPE dnsPolicy)
{
    dnsPolicyType_ = dnsPolicy;
}

void FirewallExceptions::setLocationsPingIps(const QStringList &listIps)
{
    locationsPingIPs_ = listIps;
}

void FirewallExceptions::setCustomConfigPingIps(const QStringList &listIps)
{
    customConfigsPingIPs_ = listIps;
}

QSet<QString> FirewallExceptions::getIPAddressesForFirewall() const
{
    //WS_ASSERT(QApplication::instance()->thread() == QThread::currentThread());

    UniqueIpList ipList;
    ipList.add("127.0.0.1");

    // add dns servers
    if (dnsPolicyType_ == DNS_TYPE_OS_DEFAULT) {
        std::vector<std::wstring> listDns = DnsUtils::getOSDefaultDnsServers();
        for (std::vector<std::wstring>::iterator it = listDns.begin(); it != listDns.end(); ++it) {
            ipList.add(QString::fromStdWString(*it));
        }
    } else if (dnsPolicyType_ == DNS_TYPE_OPEN_DNS) {
        for (const QString &s : HardcodedSettings::instance().openDns()) {
            ipList.add(s);
        }
    } else if (dnsPolicyType_ == DNS_TYPE_CLOUDFLARE) {
        for (const QString &s : HardcodedSettings::instance().cloudflareDns()) {
            ipList.add(s);
        }
    } else if (dnsPolicyType_ == DNS_TYPE_GOOGLE) {
        for (const QString &s : HardcodedSettings::instance().googleDns()) {
            ipList.add(s);
        }
    } else if (dnsPolicyType_ == DNS_TYPE_CONTROLD) {
        for (const QString &s : HardcodedSettings::instance().controldDns()) {
            ipList.add(s);
        }
    }

    // we should always add ControlD DNS servers because the ctrld-utility uses them
    for (const QString &s : HardcodedSettings::instance().controldDns()) {
        ipList.add(s);
    }

    for (const QString &s : hostIPs_) {
        if (!s.isEmpty()) {
            ipList.add(s);
        }
    }

    if (!proxyIP_.isEmpty()) {
        ipList.add(proxyIP_);
    }

    if (!remoteIP_.isEmpty()) {
        ipList.add(remoteIP_);
    }

    for (const QString &s : dnsIps_)
            ipList.add(s);

    for (const QString &sl : locationsPingIPs_) {
        if (!sl.isEmpty()) {
            ipList.add(sl);
        }
    }

    for (const QString &sl : customConfigsPingIPs_) {
        if (!sl.isEmpty()) {
            ipList.add(sl);
        }
    }

    return ipList.get();
}

QSet<QString> FirewallExceptions::getIPAddressesForFirewallForConnectedState() const
{
    UniqueIpList ipList;
    ipList.add("127.0.0.1");
    if (!remoteIP_.isEmpty()) {
        ipList.add(remoteIP_);
    }
    return ipList.get();
}

const QString& FirewallExceptions::connectingIp() const
{
    return connectingIp_;
}

FirewallExceptions::FirewallExceptions(): dnsPolicyType_(DNS_TYPE_OPEN_DNS)
{

}
