#pragma once

#include <QString>

#include "api_responses/amneziawgunblockparams.h"
#include "../../helper/common/helper_commands.h"

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
    QString clientListenPort() const { return client_.listenPort; }

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

    // True when the persisted fields (client keypair + server-generated peer params + client IP)
    // are sufficient to bring up a tunnel without an API round-trip. Peer public key/endpoint come
    // from the node descriptor at connect time, so they are intentionally not part of this check.
    bool haveCompleteConfig() const;

    QString generateConfigFile() const;

    bool generateKeyPair();
    bool haveKeyPair() const;
    void setKeyPair(const QString& publicKey, const QString& privateKey);

    WireGuardConfig stripIpv6Addresses() const;

    friend QDataStream& operator <<(QDataStream &stream, const WireGuardConfig &c);
    friend QDataStream& operator >>(QDataStream &stream, WireGuardConfig &c);

    bool haveAmneziawgParam() const { return client_.amneziawgParam.isValid(); }
    bool hasValidAmneziawgParams() const;
    bool hasValidServerValues() const;
    void setAmneziawgParam(const api_responses::AmneziawgUnblockParam &param) { client_.amneziawgParam = param; }
    AmneziawgConfig amneziawgParamToHelperConfig() const;
    QString amneziawgParamTitle() const { return client_.amneziawgParam.title; }

private:
    struct {
        QString privateKey;
        QString publicKey;
        QString ipAddress;
        QString dnsAddress;
        QString listenPort;
        api_responses::AmneziawgUnblockParam amneziawgParam;
    } client_;
    struct {
        QString publicKey;
        QString presharedKey;
        QString endpoint;
        QString allowedIps;
    } peer_;

    // for serialization
    static constexpr quint32 versionForSerialization_ = 2;  // should increment the version if the data format is changed
};
