#include "firewallexceptions.h"
#include "uniqueiplist.h"
#include <QThread>
#include "utils/hardcodedsettings.h"
#include "engine/dnsresolver/dnsutils.h"

void FirewallExceptions::setHostIPs(const QStringList &hostIPs)
{
    hostIPs_ = hostIPs;
}

void FirewallExceptions::setProxyIP(const ProxySettings &proxySettings)
{
    if (proxySettings.option() == PROXY_OPTION_NONE)
    {
        proxyIP_.clear();
    }
    else if (proxySettings.option() == PROXY_OPTION_HTTP || proxySettings.option() == PROXY_OPTION_SOCKS)
    {
        proxyIP_ = proxySettings.address();
    }
    else
    {
        Q_ASSERT(false);
    }
}

void FirewallExceptions::setCustomRemoteIp(const QString &remoteIP, bool &bChanged)
{
    if (remoteIP_ != remoteIP)
    {
        remoteIP_ = remoteIP;
        bChanged = true;
    }
    else
    {
        bChanged = false;
    }
}

void FirewallExceptions::setConnectingIp(const QString &connectingIp, bool &bChanged)
{
    if (connectingIp_ != connectingIp)
    {
        connectingIp_ = connectingIp;
        bChanged = true;
    }
    else
    {
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

QString FirewallExceptions::getIPAddressesForFirewall() const
{
    //Q_ASSERT(QApplication::instance()->thread() == QThread::currentThread());

    UniqueIpList ipList;
    ipList.add("127.0.0.1");

    // add dns servers
    if (dnsPolicyType_ == DNS_TYPE_OS_DEFAULT)
    {
        std::vector<std::wstring> listDns = DnsUtils::getDnsServers();
        for (std::vector<std::wstring>::iterator it = listDns.begin(); it != listDns.end(); ++it)
        {
            ipList.add(QString::fromStdWString(*it));
        }
    }
    else if (dnsPolicyType_ == DNS_TYPE_OPEN_DNS)
    {
        for (const QString &s : HardcodedSettings::instance().openDns())
        {
            ipList.add(s);
        }
    }
    else if (dnsPolicyType_ == DNS_TYPE_CLOUDFLARE)
    {
        for (const QString &s : HardcodedSettings::instance().cloudflareDns())
        {
            ipList.add(s);
        }
    }
    else if (dnsPolicyType_ == DNS_TYPE_GOOGLE)
    {
        for (const QString &s : HardcodedSettings::instance().googleDns())
        {
            ipList.add(s);
        }
    }

    // add hardcoded IPs
    for (const QString &s : HardcodedSettings::instance().apiIps())
    {
        ipList.add(s);
    }

    for (const QString &s : hostIPs_)
    {
        if (!s.isEmpty())
        {
            ipList.add(s);
        }
    }

    if (!proxyIP_.isEmpty())
    {
        ipList.add(proxyIP_);
    }

    if (!remoteIP_.isEmpty())
    {
        ipList.add(remoteIP_);
    }

    if (!connectingIp_.isEmpty())
    {
        ipList.add(connectingIp_);
    }

    for (const QString &sl : locationsPingIPs_)
    {
        if (!sl.isEmpty())
        {
            ipList.add(sl);
        }
    }

    for (const QString &sl : customConfigsPingIPs_)
    {
        if (!sl.isEmpty())
        {
            ipList.add(sl);
        }
    }

    return ipList.getFirewallString();
}

QString FirewallExceptions::getIPAddressesForFirewallForConnectedState(const QString &connectedIp) const
{
    UniqueIpList ipList;
    ipList.add("127.0.0.1");
    ipList.add(connectedIp);
    if (!remoteIP_.isEmpty())
    {
        ipList.add(remoteIP_);
    }
    return ipList.getFirewallString();
}

FirewallExceptions::FirewallExceptions(): dnsPolicyType_(DNS_TYPE_OPEN_DNS)
{

}
