#include "wireguardconfig.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QRegularExpression>

#include <system_error>

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

void WireGuardConfig::generateConfigFile(const QString &fileName) const
{
    // Design Note:
    // Tried to use QSettings(fileName, QSettings::IniFormat) to create this file.
    // Unfortunately, the setValue method double-quotes any string with non-alphanumeric
    // characters in it, such as the private/public keys which are base64 encoded.
    // The wireguard-windows service cannot handle these double-quoted entries.

    QFile theFile(fileName);
    bool bResult = theFile.open(QIODeviceBase::WriteOnly | QIODevice::Text | QIODevice::Truncate);

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

void WireGuardConfig::reset()
{
    client_.privateKey.clear();
    client_.publicKey.clear();
    client_.ipAddress.clear();
    client_.dnsAddress.clear();
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

void WireGuardConfig::setKeyPair(QString& publicKey, QString& privateKey)
{
    client_.publicKey  = publicKey;
    client_.privateKey = privateKey;
}

bool WireGuardConfig::haveServerGeneratedPeerParams() const
{
    return !peer_.presharedKey.isEmpty() && !peer_.allowedIps.isEmpty();
}
