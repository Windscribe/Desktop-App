#include "wireguardconfig.h"

#include <QJsonObject>
#include <QStringList>

WireGuardConfig::WireGuardConfig()
{

}

WireGuardConfig::WireGuardConfig(const QString &privateKey, const QString &ipAddress,
    const QString &dnsAddress, const QString &publicKey, const QString &presharedKey,
    const QString &endpoint, const QString &allowedIps)
{
    client_.privateKey = privateKey;
    client_.ipAddress = ipAddress;
    client_.dnsAddress = dnsAddress;
    peer_.publicKey = publicKey;
    peer_.presharedKey = presharedKey;
    peer_.endpoint = endpoint;
    peer_.allowedIps = allowedIps;
}

bool WireGuardConfig::initFromJson(QJsonObject &init_obj)
{
    if (!init_obj.contains("PrivateKey") ||
        !init_obj.contains("Address") ||
        !init_obj.contains("DNS"))
        return false;

    client_.privateKey = init_obj["PrivateKey"].toString();
    client_.ipAddress = WireGuardConfig::stripIpv6Address(init_obj["Address"].toString());
    client_.dnsAddress = WireGuardConfig::stripIpv6Address(init_obj["DNS"].toString());

    peer_.publicKey.clear();
    peer_.endpoint.clear();

    if (init_obj.contains("PresharedKey"))
        peer_.presharedKey = init_obj["PresharedKey"].toString();
    else
        peer_.presharedKey.clear();

    if (init_obj.contains("AllowedIPs"))
        peer_.allowedIps = WireGuardConfig::stripIpv6Address(init_obj["AllowedIPs"].toString());
    else
        peer_.allowedIps = "0.0.0.0/0";

    return true;
}

void WireGuardConfig::updatePeerInfo(const QString &publicKey, const QString &endpoint)
{
    peer_.publicKey = publicKey;
    peer_.endpoint = endpoint;
}

// static
QString WireGuardConfig::stripIpv6Address(const QStringList &addressList)
{
    const QRegExp rx("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(.*)$");
    QString s = addressList.filter(rx).join(",");
    return s;
}

// static
QString WireGuardConfig::stripIpv6Address(const QString &addressList)
{
    return stripIpv6Address(addressList.split(",", Qt::SkipEmptyParts));
}
