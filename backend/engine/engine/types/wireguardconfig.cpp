#include "wireguardconfig.h"

#include <QJsonObject>

WireGuardConfig::WireGuardConfig()
{

}

bool WireGuardConfig::initFromJson(QJsonObject &init_obj)
{
    if (!init_obj.contains("PrivateKey") ||
        !init_obj.contains("Address") ||
        !init_obj.contains("DNS") ||
        !init_obj.contains("PresharedKey"))
        return false;

    client_.privateKey = init_obj["PrivateKey"].toString();
    client_.ipAddress = init_obj["Address"].toString();
    client_.dnsAddress = init_obj["DNS"].toString();

    peer_.publicKey.clear();
    peer_.endpoint.clear();
    peer_.presharedKey = init_obj["PresharedKey"].toString();
    if (init_obj.contains("AllowedIPs"))
        peer_.allowedIps = init_obj["AllowedIPs"].toString();
    else
        peer_.allowedIps = "0.0.0.0/0";

    return true;
}

void WireGuardConfig::updatePeerInfo(const QString &publicKey, const QString &endpoint)
{
    peer_.publicKey = publicKey;
    peer_.endpoint = endpoint;
}
