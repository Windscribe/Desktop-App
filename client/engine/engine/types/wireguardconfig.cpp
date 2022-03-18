#include "wireguardconfig.h"

#include <QFile>
#include <QJsonObject>
#include <QStringList>
#include <QTextStream>

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
    return stripIpv6Address(addressList.split(",", QString::SkipEmptyParts));
}

void WireGuardConfig::generateConfigFile(const QString &fileName) const
{
    // Design Note:
    // Tried to use QSettings(fileName, QSettings::IniFormat) to create this file.
    // Unfortunately, the setValue method double-quotes any string with non-alphanumeric
    // characters in it, such as the private/public keys which are base64 encoded.
    // The wireguard-windows service cannot handle these double-quoted entries.

    QFile theFile(fileName);
    bool bResult = theFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

    if (!bResult) {
        throw std::system_error(0, std::generic_category(),
            std::string("WireGuardConfig::generateConfigFile could not create file '") + fileName.toStdString() +
            std::string("' (") + theFile.errorString().toStdString() + std::string(")"));
    }

    QTextStream ts(&theFile);
    ts << "[Interface]\n";
    ts << "PrivateKey = " << client_.privateKey << '\n';
    ts << "Address = " << client_.ipAddress << '\n';
    ts << "DNS = " << client_.dnsAddress << '\n';
    ts << '\n';
    ts << "[Peer]\n";
    ts << "PublicKey = " << peer_.publicKey << '\n';
    ts << "Endpoint = " << peer_.endpoint << '\n';
    ts << "PresharedKey = " << peer_.presharedKey << '\n';

    // wireguard-windows implements its own 'kill switch' if we pass it 0.0.0.0/0.
    // https://git.zx2c4.com/wireguard-windows/about/docs/netquirk.md
    // We're letting our helper implement that functionality.
    if (peer_.allowedIps.compare("0.0.0.0/0") == 0) {
        ts << "AllowedIPs = 0.0.0.0/1, 128.0.0.0/1\n";
    }
    else {
        ts << "AllowedIPs = " << peer_.allowedIps << '\n';
    }

    ts.flush();

    theFile.flush();
    theFile.close();
}
