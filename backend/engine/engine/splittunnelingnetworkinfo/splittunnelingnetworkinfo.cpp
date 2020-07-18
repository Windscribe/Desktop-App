#include "splittunnelingnetworkinfo.h"
#include "Utils/logger.h"
#include <QFile>

#ifdef Q_OS_MAC
    #include "Utils/macutils.h"
#endif

SplitTunnelingNetworkInfo::SplitTunnelingNetworkInfo()
{

}

void SplitTunnelingNetworkInfo::setProtocol(const ProtocolType &protocol)
{
    protocol_ = protocol;
}

void SplitTunnelingNetworkInfo::detectDefaultRoute()
{
#ifdef Q_OS_MAC
    MacUtils::getDefaultRoute(gatewayIp_, interfaceName_);
    interfaceIp_ = MacUtils::ipAddressByInterfaceName(interfaceName_);
#endif
}

void SplitTunnelingNetworkInfo::detectInfoFromOpenVpnScript()
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
                    qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromOpenVpnScript, error, unknown field name:" << line;
                    Q_ASSERT(false);
                }
            }
            else
            {
                qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromOpenVpnScript, error, can't parse line:" << line;
                Q_ASSERT(false);
            }
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac::detectInfoFromOpenVpnScript, error, can't open file:" << strTempFileWithDns;
    }
}

void SplitTunnelingNetworkInfo::setVpnAdapterName(const QString &vpnName)
{
#ifdef Q_OS_MAC
    vpnAdapterName_ = vpnName;
    ikev2AdapterAddress_ = MacUtils::ipAddressByInterfaceName(vpnAdapterName_);
#endif
}

void SplitTunnelingNetworkInfo::setIkev2DnsServers(const QStringList &dnsList)
{
    dnsServers_ = dnsList;
}

void SplitTunnelingNetworkInfo::setConnectedIp(const QString &ip)
{
    connectedIp_ = ip;
}

ProtocolType SplitTunnelingNetworkInfo::protocol() const
{
    return protocol_;
}

QString SplitTunnelingNetworkInfo::gatewayIp() const
{
    return gatewayIp_;
}

QString SplitTunnelingNetworkInfo::interfaceName() const
{
    return interfaceName_;
}

QString SplitTunnelingNetworkInfo::interfaceIp() const
{
    return interfaceIp_;
}

QString SplitTunnelingNetworkInfo::connectedIp() const
{
    return connectedIp_;
}

QString SplitTunnelingNetworkInfo::routeVpnGateway() const
{
    return routeVpnGateway_;
}

QString SplitTunnelingNetworkInfo::routeNetGateway() const
{
    return routeNetGateway_;
}

QString SplitTunnelingNetworkInfo::remote1() const
{
    return remote1_;
}

QString SplitTunnelingNetworkInfo::ifconfigTunIp() const
{
    return ifconfigTunIp_;
}

QString SplitTunnelingNetworkInfo::vpnAdapterName() const
{
    return vpnAdapterName_;
}

QString SplitTunnelingNetworkInfo::ikev2AdapterAddress() const
{
    return ikev2AdapterAddress_;
}

QStringList SplitTunnelingNetworkInfo::dnsServers() const
{
    return dnsServers_;
}

void SplitTunnelingNetworkInfo::outToLog()
{
    qCDebug(LOG_BASIC) << "SplitTunnelingNetworkInfo_mac, gatewayIp:" << gatewayIp_ << "interfaceName:" << interfaceName_
                       << "interfaceIp:" << interfaceIp_ << "connectedIp:" << connectedIp_ << "route_vpn_gateway:" << routeVpnGateway_
                       << "route_net_gateway:" << routeNetGateway_
                       << "remote1:" << remote1_
                       << "vpnAdapterName:" << vpnAdapterName_ << "ikev2AdapterAddress:" << ikev2AdapterAddress_
                       << "dnsServers:" << dnsServers_;
}

bool SplitTunnelingNetworkInfo::parseLine(const QString &line, QString &fieldName, QString &value)
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
