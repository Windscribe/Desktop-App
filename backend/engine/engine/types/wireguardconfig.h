#ifndef WIREGUARDCONFIG_H
#define WIREGUARDCONFIG_H

#include <QJsonObject>
#include <QString>

class WireGuardConfig
{
public:
    WireGuardConfig();

    bool initFromJson(QJsonObject &obj);
    void updatePeerInfo(const QString &publicKey, const QString &endpoint);

    QString clientPrivateKey() const { return client_.privateKey; }
    QString clientIpAddress() const { return client_.ipAddress; }
    QString clientDnsAddress() const { return client_.dnsAddress; }

    QString peerPublicKey() const { return peer_.publicKey; }
    QString peerPresharedKey() const { return peer_.presharedKey; }
    QString peerEndpoint() const { return peer_.endpoint; }
    QString peerAllowedIps() const { return peer_.allowedIps; }

private:
    struct {
        QString privateKey;
        QString ipAddress;
        QString dnsAddress;
    } client_;
    struct {
        QString publicKey;
        QString presharedKey;
        QString endpoint;
        QString allowedIps;
    } peer_;
};

#endif // WIREGUARDCONFIG_H
