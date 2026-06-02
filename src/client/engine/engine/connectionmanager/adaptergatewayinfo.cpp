#include "adaptergatewayinfo.h"

#include <QMetaType>
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
    #include "adapterutils_win.h"
#elif defined Q_OS_MACOS
    #include "utils/network_utils/network_utils_mac.h"
#elif defined Q_OS_LINUX
    #include "utils/network_utils/network_utils_linux.h"
#endif

const int typeIdAdapterGatewayInfo = qRegisterMetaType<AdapterGatewayInfo>("AdapterGatewayInfo");


AdapterGatewayInfo AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo()
{
    AdapterGatewayInfo cai;

#ifdef Q_OS_WIN
    cai = AdapterUtils_win::getDefaultAdapterInfo();
#elif defined Q_OS_MACOS
    QString gateway;
    NetworkUtils_mac::getDefaultRoute(gateway, cai.adapterName_);
    cai.addGatewayIp(types::IpAddress(gateway.toStdString()));
    cai.addAdapterIp(types::IpAddress(NetworkUtils_mac::ipAddressByInterfaceName(cai.adapterName_).toStdString()));

    // IPv6 default route. Independent of the v4 default — picked from `netstat -nr -f inet6` — so
    // a dual-stack host gets both addresses populated, a v4-only host gets only v4, and a host
    // with two separate default-route interfaces per family is supported. If the v6 default-route
    // interface name differs from the v4 one, we don't override cai.adapterName_ (v4 wins as the
    // canonical name); the addresses still flow through addGatewayIp/addAdapterIp. On a v6-only
    // host (no v4 default route at all) we fall back to the v6 adapter name so isEmpty() does not
    // mistakenly treat the host as "no default adapter detected" and the connection manager can
    // proceed instead of looping in waitForNetworkConnectivity().
    {
        QString gatewayV6, adapterNameV6;
        NetworkUtils_mac::getDefaultRouteV6(gatewayV6, adapterNameV6);
        cai.addGatewayIp(types::IpAddress(gatewayV6.toStdString()));
        cai.addAdapterIp(types::IpAddress(NetworkUtils_mac::ipAddressByInterfaceNameV6(adapterNameV6).toStdString()));
        cai.setAdapterNameV6(adapterNameV6);
        if (cai.adapterName_.isEmpty()) {
            cai.adapterName_ = adapterNameV6;
        }
    }

    // DNS pulls from scutil --dns, which on a dual-stack host returns both v4 and v6 nameservers
    // verbatim. types::IpAddress::IpAddress(const std::string &) rejects scope-suffixed
    // link-local DNS (`fe80::1%en0`) by yielding an invalid address, which addDnsServer drops; no
    // explicit filter needed here.
    const QStringList dnsStrings = NetworkUtils_mac::getDnsServersForInterface(cai.adapterName_);
    for (const QString &s : dnsStrings)
        cai.addDnsServer(types::IpAddress(s.toStdString()));
#elif defined Q_OS_LINUX
    QString gateway, adapterIp;
    NetworkUtils_linux::getDefaultRoute(gateway, cai.adapterName_, adapterIp);
    cai.addGatewayIp(types::IpAddress(gateway.toStdString()));
    cai.addAdapterIp(types::IpAddress(adapterIp.toStdString()));

    // IPv6 default route. Independent of the v4 default — picked from /proc/net/ipv6_route — so
    // a dual-stack host gets both addresses populated, a v4-only host gets only v4, and a host
    // with two separate default-route interfaces per family is supported. If the v6 default-route
    // interface name differs from the v4 one, we don't override cai.adapterName_ (v4 wins as the
    // canonical name); the addresses still flow through addGatewayIp/addAdapterIp. The v6
    // interface name is kept separately (adapterNameV6_) so split-tunnel v6 routes use the
    // correct `dev` on a multi-homed host where v6 default lives on a different NIC than v4.
    {
        QString gatewayV6, adapterNameV6, adapterIpV6;
        NetworkUtils_linux::getDefaultRouteV6(gatewayV6, adapterNameV6, adapterIpV6);
        cai.addGatewayIp(types::IpAddress(gatewayV6.toStdString()));
        cai.addAdapterIp(types::IpAddress(adapterIpV6.toStdString()));
        cai.setAdapterNameV6(adapterNameV6);
    }

    // DNS servers intentionally not populated on Linux: the current helper does not consume
    // defaultAdapter.dnsServers (no read sites under src/helper/linux), so reading /etc/resolv.conf
    // here would be dead data. macOS keeps its own getDnsServersForInterface path because IKEv2
    // and other macOS-specific helper code consumes that field. If a future Linux helper feature
    // needs system DNS, parse /etc/resolv.conf here at the same time it gains a consumer.
#endif

    return cai;
}

AdapterGatewayInfo::AdapterGatewayInfo() : ifIndex_(0)
{
}

void AdapterGatewayInfo::addAdapterIp(const types::IpAddress &ip)
{
    if (ip.isValid()) {
        adapterIps_ << ip;
    }
}

void AdapterGatewayInfo::addGatewayIp(const types::IpAddress &ip)
{
    if (ip.isValid()) {
        gatewayIps_ << ip;
    }
}

void AdapterGatewayInfo::addDnsServer(const types::IpAddress &ip)
{
    if (ip.isValid()) {
        dnsServers_.append(ip);
    }
}

types::IpAddress AdapterGatewayInfo::adapterIpV4() const
{
    for (const auto &ip : adapterIps_) {
        if (ip.isV4()) {
            return ip;
        }
    }
    return types::IpAddress();
}

types::IpAddress AdapterGatewayInfo::adapterIpV6() const
{
    for (const auto &ip : adapterIps_) {
        if (ip.isV6()) {
            return ip;
        }
    }
    return types::IpAddress();
}

types::IpAddress AdapterGatewayInfo::gatewayV4() const
{
    for (const auto &ip : gatewayIps_) {
        if (ip.isV4()) {
            return ip;
        }
    }
    return types::IpAddress();

}

types::IpAddress AdapterGatewayInfo::gatewayV6() const
{
    for (const auto &ip : gatewayIps_) {
        if (ip.isV6()) {
            return ip;
        }
    }
    return types::IpAddress();
}

bool AdapterGatewayInfo::isEmpty() const
{
    return adapterName_.isEmpty();
}

QStringList AdapterGatewayInfo::dnsServersAsStringList() const
{
    QStringList result;
    for (const auto &ip : dnsServers_)
        result << QString::fromStdString(ip.toString());
    return result;
}

QString AdapterGatewayInfo::makeLogString()
{
    auto joinIps = [](const QVector<types::IpAddress> &ips) {
        QStringList strs;
        for (const auto &ip : ips)
            strs << QString::fromStdString(ip.toString());
        return strs.join(',');
    };

    return QString("adapter name = %1 (v6: %2); adapter IPs = (%3); gateway IPs = (%4);"
                   " remote IP = %5; dns = (%6); ifIndex = %7")
            .arg(adapterName_)
            .arg(adapterNameV6())
            .arg(joinIps(adapterIps_))
            .arg(joinIps(gatewayIps_))
            .arg(QString::fromStdString(remoteIp_.toString()))
            .arg(joinIps(dnsServers_))
            .arg(ifIndex_ != 0 ? QString::number(ifIndex_) : "not detected");
}
