#include "wireguardconfig.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/openssl_utils.h"

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
    client_.listenPort = QString();
    peer_.publicKey = publicKey;
    peer_.presharedKey = presharedKey;
    peer_.endpoint = endpoint;
    peer_.allowedIps = allowedIps;
}

// static
QString WireGuardConfig::stripIpv6Address(const QStringList &addressList)
{
    const QRegularExpression rx("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(.*)$");
    QString s = addressList.filter(rx).join(",");
    return s;
}

// static
QString WireGuardConfig::stripIpv6Address(const QString &addressList)
{
    return stripIpv6Address(addressList.split(",", Qt::SkipEmptyParts));
}

QString WireGuardConfig::generateConfigFile() const
{
    // Design Note:
    // Tried to use QSettings(fileName, QSettings::IniFormat) to create this file.
    // Unfortunately, the setValue method double-quotes any string with non-alphanumeric
    // characters in it, such as the private/public keys which are base64 encoded.
    // The wireguard-windows service cannot handle these double-quoted entries.

    QString config;
    QTextStream ts(&config);
    ts << "[Interface]\n";
    ts << "PrivateKey = " << client_.privateKey << '\n';
    ts << "Address = " << client_.ipAddress << '\n';
    ts << "DNS = " << client_.dnsAddress << '\n';

    if (!client_.listenPort.isEmpty()) {
        ts << "ListenPort = " << client_.listenPort << '\n';
    }

    if (client_.amneziawgParam.isValid()) {
        if (client_.amneziawgParam.jc > 0) {
            ts << "Jc = " << client_.amneziawgParam.jc << '\n';
        }
        if (client_.amneziawgParam.jmin > 0) {
            ts << "Jmin = " << client_.amneziawgParam.jmin << '\n';
        }
        if (client_.amneziawgParam.jmax > 0) {
            ts << "Jmax = " << client_.amneziawgParam.jmax << '\n';
        }
        if (client_.amneziawgParam.s1 != 0) {
            ts << "S1 = " << client_.amneziawgParam.s1 << '\n';
        }
        if (client_.amneziawgParam.s2 != 0) {
            ts << "S2 = " << client_.amneziawgParam.s2 << '\n';
        }
        if (client_.amneziawgParam.s3 != 0) {
            ts << "S3 = " << client_.amneziawgParam.s3 << '\n';
        }
        if (client_.amneziawgParam.s4 != 0) {
            ts << "S4 = " << client_.amneziawgParam.s4 << '\n';
        }
        if (!client_.amneziawgParam.h1.isEmpty()) {
            ts << "H1 = " << client_.amneziawgParam.h1 << '\n';
        }
        if (!client_.amneziawgParam.h2.isEmpty()) {
            ts << "H2 = " << client_.amneziawgParam.h2 << '\n';
        }
        if (!client_.amneziawgParam.h3.isEmpty()) {
            ts << "H3 = " << client_.amneziawgParam.h3 << '\n';
        }
        if (!client_.amneziawgParam.h4.isEmpty()) {
            ts << "H4 = " << client_.amneziawgParam.h4 << '\n';
        }

        // Send I1-I5 parameters (in order, only if they exist)
        for (auto i = 0; i < client_.amneziawgParam.iValues.size() && i < 5; ++i) {
            if (!client_.amneziawgParam.iValues[i].isEmpty()) {
                ts << "I" << i + 1 << " = " << client_.amneziawgParam.iValues[i] << '\n';
            }
        }
    }

    ts << '\n';
    ts << "[Peer]\n";
    ts << "PublicKey = " << peer_.publicKey << '\n';
    ts << "Endpoint = " << peer_.endpoint << '\n';

    if (!peer_.presharedKey.isEmpty()) {
        ts << "PresharedKey = " << peer_.presharedKey << '\n';
    }

    // wireguard-windows implements its own 'kill switch' if we pass it 0.0.0.0/0.
    // https://git.zx2c4.com/wireguard-windows/about/docs/netquirk.md
    // We're letting our helper implement that functionality.
    if (peer_.allowedIps.compare("0.0.0.0/0") == 0) {
        ts << "AllowedIPs = 0.0.0.0/1, 128.0.0.0/1\n";
    }
    else {
        ts << "AllowedIPs = " << peer_.allowedIps << '\n';
    }
    // PersistentKeepalive is needed to force handshake right after
    // the interface is configured. Otherwise, Wireguard waits for any incoming
    // packet to the network interface, which interfere with the blocking firewall.
    ts << "PersistentKeepalive = 25\n";
    ts.flush();

    return config;
}

void WireGuardConfig::reset()
{
    client_.privateKey.clear();
    client_.publicKey.clear();
    client_.ipAddress.clear();
    client_.dnsAddress.clear();
    client_.listenPort.clear();
    client_.amneziawgParam = api_responses::AmneziawgUnblockParam();
    peer_.publicKey.clear();
    peer_.presharedKey.clear();
    peer_.endpoint.clear();
    peer_.allowedIps.clear();
}

bool WireGuardConfig::generateKeyPair()
{
    reset();

    wsl::EvpPkeyCtx ctxKey(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL));

    if (!ctxKey.isValid()) {
        qCCritical(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_CTX_new_id failed";
        return false;
    }

    int nResult = EVP_PKEY_keygen_init(ctxKey.context());

    if (nResult <= 0) {
        qCCritical(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_keygen_init failed:" << nResult;
        return false;
    }

    wsl::EvpPkey keyX25519;
    nResult = EVP_PKEY_keygen(ctxKey.context(), keyX25519.ppkey());

    if (nResult <= 0) {
        qCCritical(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_keygen failed:" << nResult;
        return false;
    }

    unsigned char keyBuf[64];
    size_t keyBufLen = 64;
    nResult = EVP_PKEY_get_raw_private_key(keyX25519.pkey(), keyBuf, &keyBufLen);

    if (nResult <= 0) {
        qCCritical(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_get_raw_private_key failed:" << nResult;
        return false;
    }

    QByteArray privateKey((const char*)keyBuf, keyBufLen);

    keyBufLen = 64;
    nResult = EVP_PKEY_get_raw_public_key(keyX25519.pkey(), keyBuf, &keyBufLen);

    if (nResult <= 0) {
        qCCritical(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_get_raw_public_key failed:" << nResult;
        return false;
    }

    QByteArray publicKey((const char*)keyBuf, keyBufLen);

    client_.privateKey = privateKey.toBase64();
    client_.publicKey  = publicKey.toBase64();

    return true;
}

bool WireGuardConfig::haveKeyPair() const
{
    return !client_.privateKey.isEmpty() && !client_.publicKey.isEmpty();
}

void WireGuardConfig::setKeyPair(const QString &publicKey, const QString &privateKey)
{
    client_.publicKey  = publicKey;
    client_.privateKey = privateKey;
}

bool WireGuardConfig::haveServerGeneratedPeerParams() const
{
    return !peer_.presharedKey.isEmpty() && !peer_.allowedIps.isEmpty();
}

AmneziawgConfig WireGuardConfig::amneziawgParamToHelperConfig() const
{
    AmneziawgConfig config;
    if (client_.amneziawgParam.isValid()) {
        config.title = client_.amneziawgParam.title.toStdString();
        config.jc = client_.amneziawgParam.jc;
        config.jmin = client_.amneziawgParam.jmin;
        config.jmax = client_.amneziawgParam.jmax;
        config.s1 = client_.amneziawgParam.s1;
        config.s2 = client_.amneziawgParam.s2;
        config.s3 = client_.amneziawgParam.s3;
        config.s4 = client_.amneziawgParam.s4;
        config.h1 = client_.amneziawgParam.h1.toStdString();
        config.h2 = client_.amneziawgParam.h2.toStdString();
        config.h3 = client_.amneziawgParam.h3.toStdString();
        config.h4 = client_.amneziawgParam.h4.toStdString();
        for (const auto &iValue : client_.amneziawgParam.iValues) {
            config.iValues.push_back(iValue.toStdString());
        }
        if (ExtraConfig::instance().getWireGuardVerboseLogging()) {
            qCDebug(LOG_CONNECTION) << "WireGuardConfig::amneziawgParamToHelperConfig" << client_.amneziawgParam;
        }
    }
    return config;
}

QDataStream& operator <<(QDataStream &stream, const WireGuardConfig &c)
{
    // There should be no need to serialize the amneziawgParam, as it contains information that is already persisted
    // in wsnet (available configurations from the server) and EngineSettings (the config the user has selected).
    // We only persist here data that is unique to this class.
    stream << c.versionForSerialization_;
    stream << c.client_.privateKey << c.client_.publicKey << c.client_.ipAddress << c.client_.dnsAddress << c.client_.listenPort;
    stream << c.peer_.publicKey << c.peer_.presharedKey << c.peer_.endpoint << c.peer_.allowedIps;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, WireGuardConfig &c)
{
    quint32 version;
    stream >> version;
    if (version > c.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }

    stream >> c.client_.privateKey >> c.client_.publicKey >> c.client_.ipAddress >> c.client_.dnsAddress;
    if (version >= 2) {
        stream >> c.client_.listenPort;
    }
    stream >> c.peer_.publicKey >> c.peer_.presharedKey >> c.peer_.endpoint >> c.peer_.allowedIps;
    return stream;
}
