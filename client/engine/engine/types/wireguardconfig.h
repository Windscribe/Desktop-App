#ifndef WIREGUARDCONFIG_H
#define WIREGUARDCONFIG_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>

class WireGuardConfig
{
public:
    WireGuardConfig();
    WireGuardConfig(const QString &privateKey, const QString &ipAddress, const QString &dnsAddress,
                    const QString &publicKey, const QString &presharedKey, const QString &endpoint,
                    const QString &allowedIps);

    bool initFromJson(QJsonObject &obj);
    void updatePeerInfo(const QString &publicKey, const QString &endpoint);
    void reset();

    QString clientPrivateKey() const { return client_.privateKey; }
    QString clientIpAddress() const { return client_.ipAddress; }
    QString clientDnsAddress() const { return client_.dnsAddress; }

    QString peerPublicKey() const { return peer_.publicKey; }
    QString peerPresharedKey() const { return peer_.presharedKey; }
    QString peerEndpoint() const { return peer_.endpoint; }
    QString peerAllowedIps() const { return peer_.allowedIps; }

    void generateConfigFile(const QString &fileName) const;

    bool generateKeyPair();

    static QString stripIpv6Address(const QStringList &addressList);
    static QString stripIpv6Address(const QString &addressList);

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

    QDateTime keyPairGeneratedTimestamp_;
};

#endif // WIREGUARDCONFIG_H
