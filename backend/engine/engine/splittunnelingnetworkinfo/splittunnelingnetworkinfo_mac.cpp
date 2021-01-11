#include "splittunnelingnetworkinfo_mac.h"
#include "utils/logger.h"
#include <QFile>

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

SplitTunnelingNetworkInfo_mac::SplitTunnelingNetworkInfo_mac()
{

}

void SplitTunnelingNetworkInfo_mac::setProtocol(const ProtocolType &protocol)
{
    protocol_ = protocol;
}

void SplitTunnelingNetworkInfo_mac::detectDefaultRoute()
{
#ifdef Q_OS_MAC
    MacUtils::getDefaultRoute(gatewayIp_, interfaceName_);
    interfaceIp_ = MacUtils::ipAddressByInterfaceName(interfaceName_);
#endif
}

void SplitTunnelingNetworkInfo_mac::detectInfoFromDnsScript()
{
    dnsServers_.clear();
    // dns servers written by script dns.sh to file $TMPDIR/windscribe_dns.aaa
    QString strTempFileWithDns = QString::fromLocal8Bit(getenv("TMPDIR"));
    strTempFileWithDns = strTempFileWithDns + "windscribe_dns.aaa";
    QFile file(strTempFileWithDns);
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QString fieldName, value;
            if (parseLine(line, fieldName, value))
            {
                if (fieldName == "dns_servers")
                {
                    dnsServers_ << value;
                }
                else if (fieldName == "route_vpn_gateway")
                {
                    routeVpnGateway_ = value;
                }
                else if (fieldName == "route_net_gateway")
                {
                    routeNetGateway_ = value;
                }
                else if (fieldName == "remote_1")
                {
                    remote1_ = value;
                }
                else if (fieldName == "ifconfig_local")
                {
                    ifconfigTunIp_ = value;
                }
                else
                {
                    qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromDnsScript, error, unknown field name:" << line;
                    Q_ASSERT(false);
                }
            }
            else
            {
                qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromDnsScript, error, can't parse line:" << line;
                Q_ASSERT(false);
            }
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromDnsScript, error, can't open file:" << strTempFileWithDns;
    }
}

void SplitTunnelingNetworkInfo_mac::setVpnAdapterName(const QString &vpnName)
{
#ifdef Q_OS_MAC
    vpnAdapterName_ = vpnName;
    vpnAdapterIpAddress_ = MacUtils::ipAddressByInterfaceName(vpnAdapterName_);
#else
    Q_UNUSED(vpnName);
#endif
}

void SplitTunnelingNetworkInfo_mac::setIkev2DnsServers(const QStringList &dnsList)
{
    dnsServers_ = dnsList;
}

void SplitTunnelingNetworkInfo_mac::setConnectedIp(const QString &ip)
{
    connectedIp_ = ip;
}

ProtocolType SplitTunnelingNetworkInfo_mac::protocol() const
{
    return protocol_;
}

QString SplitTunnelingNetworkInfo_mac::gatewayIp() const
{
    return gatewayIp_;
}

QString SplitTunnelingNetworkInfo_mac::interfaceName() const
{
    return interfaceName_;
}

QString SplitTunnelingNetworkInfo_mac::interfaceIp() const
{
    return interfaceIp_;
}

QString SplitTunnelingNetworkInfo_mac::connectedIp() const
{
    return connectedIp_;
}

QString SplitTunnelingNetworkInfo_mac::routeVpnGateway() const
{
    return routeVpnGateway_;
}

QString SplitTunnelingNetworkInfo_mac::routeNetGateway() const
{
    return routeNetGateway_;
}

QString SplitTunnelingNetworkInfo_mac::remote1() const
{
    return remote1_;
}

QString SplitTunnelingNetworkInfo_mac::ifconfigTunIp() const
{
    return ifconfigTunIp_;
}

QString SplitTunnelingNetworkInfo_mac::vpnAdapterName() const
{
    return vpnAdapterName_;
}

QString SplitTunnelingNetworkInfo_mac::vpnAdapterIpAddress() const
{
    return vpnAdapterIpAddress_;
}

QStringList SplitTunnelingNetworkInfo_mac::dnsServers() const
{
    return dnsServers_;
}

void SplitTunnelingNetworkInfo_mac::outToLog() const
{
    qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac, gatewayIp:" << gatewayIp_ << "interfaceName:" << interfaceName_
                       << "interfaceIp:" << interfaceIp_ << "connectedIp:" << connectedIp_ << "route_vpn_gateway:" << routeVpnGateway_
                       << "route_net_gateway:" << routeNetGateway_
                       << "remote1:" << remote1_
                       << "vpnAdapterName:" << vpnAdapterName_ << "vpnAdapterIpAddress:" << vpnAdapterIpAddress_
                       << "dnsServers:" << dnsServers_;
}

bool SplitTunnelingNetworkInfo_mac::parseLine(const QString &line, QString &fieldName, QString &value)
{
    QStringList list = line.split('=');
    if (list.count() == 2)
    {
        fieldName = list[0].trimmed();
        value = list[1].trimmed();
        return true;
    }
    return false;
}
