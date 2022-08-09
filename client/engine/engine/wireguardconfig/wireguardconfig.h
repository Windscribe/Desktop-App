#ifndef WIREGUARDCONFIG_H
#define WIREGUARDCONFIG_H

#include <QString>

class WireGuardConfig
{
public:
    WireGuardConfig();
    WireGuardConfig(const QString &privateKey, const QString &ipAddress, const QString &dnsAddress,
                    const QString &publicKey, const QString &presharedKey, const QString &endpoint,
                    const QString &allowedIps);

    void reset();

    QString clientPrivateKey() const { return client_.privateKey; }
    QString clientPublicKey() const { return client_.publicKey; }
    QString clientIpAddress() const { return client_.ipAddress; }
    QString clientDnsAddress() const { return client_.dnsAddress; }

    QString peerPublicKey() const { return peer_.publicKey; }
    QString peerPresharedKey() const { return peer_.presharedKey; }
    QString peerEndpoint() const { return peer_.endpoint; }
    QString peerAllowedIps() const { return peer_.allowedIps; }
    void setPeerPublicKey(const QString &publicKey) { peer_.publicKey = publicKey; }
    void setPeerEndpoint(const QString &endpoint) { peer_.endpoint = endpoint; }
    void setPeerPresharedKey(const QString &presharedKey) { peer_.presharedKey = presharedKey; }
    void setPeerAllowedIPs(const QString &allowedIPs) { peer_.allowedIps = allowedIPs; }
    void setClientIpAddress(const QString &ip) { client_.ipAddress = ip; }
    void setClientDnsAddress(const QString &dns) { client_.dnsAddress = dns; }
    bool haveServerGeneratedPeerParams() const;

    void generateConfigFile(const QString &fileName) const;

    bool generateKeyPair();
    bool haveKeyPair() const;
    void setKeyPair(const QString& publicKey, const QString& privateKey);

    static QString stripIpv6Address(const QStringList &addressList);
    static QString stripIpv6Address(const QString &addressList);

    friend QDataStream& operator <<(QDataStream &stream, const WireGuardConfig &c);
    friend QDataStream& operator >>(QDataStream &stream, WireGuardConfig &c);

private:
    struct {
        QString privateKey;
        QString publicKey;
        QString ipAddress;
        QString dnsAddress;
    } client_;
    struct {
        QString publicKey;
        QString presharedKey;
        QString endpoint;
        QString allowedIps;
    } peer_;

    // for serialization
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

#endif // WIREGUARDCONFIG_H
