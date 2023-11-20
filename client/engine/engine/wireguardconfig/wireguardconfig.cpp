#include "wireguardconfig.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include "utils/logger.h"
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

bool WireGuardConfig::generateConfigFile(const QString &fileName) const
{
    // Design Note:
    // Tried to use QSettings(fileName, QSettings::IniFormat) to create this file.
    // Unfortunately, the setValue method double-quotes any string with non-alphanumeric
    // characters in it, such as the private/public keys which are base64 encoded.
    // The wireguard-windows service cannot handle these double-quoted entries.

    QFile theFile(fileName);
    if (!theFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateConfigFile could not create file '" << fileName << "' (" << theFile.errorString() << ")";
        return false;
    }

    QTextStream ts(&theFile);
    ts << "[Interface]\n";
    ts << "PrivateKey = " << client_.privateKey << '\n';
    ts << "Address = " << client_.ipAddress << '\n';
    ts << "DNS = " << client_.dnsAddress << '\n';

    if (!client_.listenPort.isEmpty()) {
        ts << "ListenPort = " << client_.listenPort << '\n';
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

    ts.flush();

    theFile.flush();
    theFile.close();

    return true;
}

void WireGuardConfig::reset()
{
    client_.privateKey.clear();
    client_.publicKey.clear();
    client_.ipAddress.clear();
    client_.dnsAddress.clear();
    client_.listenPort.clear();
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
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_CTX_new_id failed";
        return false;
    }

    int nResult = EVP_PKEY_keygen_init(ctxKey.context());

    if (nResult <= 0) {
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_keygen_init failed:" << nResult;
        return false;
    }

    wsl::EvpPkey keyX25519;
    nResult = EVP_PKEY_keygen(ctxKey.context(), keyX25519.ppkey());

    if (nResult <= 0) {
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_keygen failed:" << nResult;
        return false;
    }

    unsigned char keyBuf[64];
    size_t keyBufLen = 64;
    nResult = EVP_PKEY_get_raw_private_key(keyX25519.pkey(), keyBuf, &keyBufLen);

    if (nResult <= 0) {
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_get_raw_private_key failed:" << nResult;
        return false;
    }

    QByteArray privateKey((const char*)keyBuf, keyBufLen);

    keyBufLen = 64;
    nResult = EVP_PKEY_get_raw_public_key(keyX25519.pkey(), keyBuf, &keyBufLen);

    if (nResult <= 0) {
        qCDebug(LOG_CONNECTION) << "WireGuardConfig::generateKeyPair - EVP_PKEY_get_raw_public_key failed:" << nResult;
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

QDataStream& operator <<(QDataStream &stream, const WireGuardConfig &c)
{
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
