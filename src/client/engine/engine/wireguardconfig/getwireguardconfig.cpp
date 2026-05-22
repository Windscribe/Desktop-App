#include "getwireguardconfig.h"
#include <QDataStream>
#include <QHostAddress>
#include <QIODevice>
#include <QSettings>
#include "api_responses/wgconfigs_init.h"
#include "types/global_consts.h"
#include "utils/log/categories.h"
#include "utils/openssl_utils.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"


const QString GetWireGuardConfig::KEY_WIREGUARD_CONFIG = "wireguardConfig";

// After each program launch we must force call wgConfigsInit
// This static variable serves this purpose
bool GetWireGuardConfig::isInitConfigWasCallAtleastOnce_ = false;

bool GetWireGuardConfig::forceReinitOnNextCall_ = false;

using namespace wsnet;

GetWireGuardConfig::GetWireGuardConfig(QObject *parent) : QObject(parent),
    simpleCrypt_(SIMPLE_CRYPT_KEY)
{
}

GetWireGuardConfig::~GetWireGuardConfig()
{
    SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(request_);
}

void GetWireGuardConfig::getWireGuardConfig(const QString &serverName, bool deleteOldestKey)
{
    WS_ASSERT(request_ == nullptr);

    serverName_ = serverName;
    deleteOldestKey_ = deleteOldestKey;
    isRetryInitRequest_ = false;

    wireGuardConfig_.reset();
    WireGuardConfig storedConfig = readWireGuardConfigFromSettings();
    const bool forceReinit = forceReinitOnNextCall_;
    forceReinitOnNextCall_ = false;
    if (!forceReinit && isInitConfigWasCallAtleastOnce_ && storedConfig.haveKeyPair() &&
        storedConfig.haveServerGeneratedPeerParams() && !storedConfig.clientIpAddress().isEmpty()) {
        wireGuardConfig_ = storedConfig;
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, wireGuardConfig_);
    } else if (storedConfig.haveKeyPair()) {
        wireGuardConfig_.setKeyPair(storedConfig.clientPublicKey(), storedConfig.clientPrivateKey());
        submitWireGuardInitRequest(false);
    } else {
        submitWireGuardInitRequest(true);
    }
}

void GetWireGuardConfig::forceReinitOnNextCall()
{
    forceReinitOnNextCall_ = true;
}

void GetWireGuardConfig::onWgConfigsInitAnswer(wsnet::ApiRetCode serverApiRetCode, const std::string &jsonData)
{
    request_.reset();

    if (serverApiRetCode == ServerApiRetCode::kFailoverFailed) {
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailoverFailed, wireGuardConfig_);
        return;
    } else if (serverApiRetCode != ServerApiRetCode::kSuccess) {
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    api_responses::WgConfigsInit res(jsonData);

    if (res.isErrorCode()) {
        WireGuardConfigRetCode newRetCode = WireGuardConfigRetCode::kFailed;
        if (res.errorCode() == 1310) {
            // This error indicates the server was unable to generate the preshared key.
            // Retry the init request one time, then abort the WireGuard connection attempt.
            if (!isRetryInitRequest_) {
                isRetryInitRequest_ = true;
                submitWireGuardInitRequest(false);
                return;
            }
        } else if (res.errorCode() == 1313) {
            // This error indicates the user has used up all of their public key slots on the server.
            // Ask them if they want to delete their oldest registered key and try again.
            newRetCode = WireGuardConfigRetCode::kKeyLimit;
        }

        if (newRetCode != WireGuardConfigRetCode::kKeyLimit) {
            qCDebug(LOG_WIREGUARD) << "Failed to get WG config, code: " << res.errorCode();
        }
        emit getWireGuardConfigAnswer(newRetCode, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setPeerPresharedKey(res.presharedKey());

    // The API returns AllowedIPs (v4) and optionally AllowedIPsV6 as separate fields, but
    // WireGuard expects a single comma-separated dual-stack list in [Peer].AllowedIPs.
    // Downstream helpers (stripIpv6Addresses, expandCatchAllRoutes) already operate on a
    // mixed v4/v6 string, so we merge here and keep the rest of the pipeline untouched.
    QString peerAllowedIps = res.allowedIps();
    const QString peerAllowedIpsV6 = res.allowedIpsV6();
    if (!peerAllowedIpsV6.isEmpty()) {
        if (!peerAllowedIps.isEmpty()) {
            peerAllowedIps += QStringLiteral(", ");
        }
        peerAllowedIps += peerAllowedIpsV6;
    }
    wireGuardConfig_.setPeerAllowedIPs(peerAllowedIps);

    if (res.hashedCIDR().isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Failed to get WG config: HashedCIDR is missing";
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    QString cidr = res.hashedCIDR().at(0);
    QString clientAddress = generateClientAddress(cidr);
    if (clientAddress.isEmpty()) {
        qCDebug(LOG_WIREGUARD) << "Failed to generate client address from CIDR:" << cidr;
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    if (!res.hashedCIDRv6().isEmpty()) {
        QString cidrV6 = res.hashedCIDRv6().at(0);
        QString clientIpv6Address = generateClientIpv6Address(cidrV6);
        if (clientIpv6Address.isEmpty()) {
            qCDebug(LOG_WIREGUARD) << "Failed to generate client IPv6 address from CIDR:" << cidrV6;
        } else {
            clientAddress += "," + clientIpv6Address;
        }
    }
    wireGuardConfig_.setClientIpAddress(clientAddress);
    wireGuardConfig_.setClientDnsAddress("10.255.255.1");

    writeWireGuardConfigToSettings(wireGuardConfig_);
    isInitConfigWasCallAtleastOnce_ = true;

    emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, wireGuardConfig_);
}

void GetWireGuardConfig::submitWireGuardInitRequest(bool generateKeyPair)
{
    qCDebug(LOG_WIREGUARD) << "Calling WireGuard init with" << (generateKeyPair ? "new" : "existing") << "keypair";
    if (generateKeyPair) {
        if (!wireGuardConfig_.generateKeyPair()) {
            request_ = nullptr;
            emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
            return;
        }
        // Persist the key-pair we're about to register with the server.
        setWireGuardKeyPair(wireGuardConfig_.clientPublicKey(), wireGuardConfig_.clientPrivateKey());
    }
    WS_ASSERT(request_ == nullptr);
    request_ = WSNet::instance()->serverAPI()->wgConfigsInit(WSNet::instance()->apiResourcersManager()->authHash(), wireGuardConfig_.clientPublicKey().toStdString(),
                                                                deleteOldestKey_,
                                                                [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
                                                                {
                                                                    QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData]() { // NOLINT: false positive for memory leak
                                                                        onWgConfigsInitAnswer(serverApiRetCode, jsonData);
                                                                    });
                                                                });
}

void GetWireGuardConfig::setWireGuardKeyPair(const QString &publicKey, const QString &privateKey)
{
    WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    wgConfig.setKeyPair(publicKey, privateKey);
    writeWireGuardConfigToSettings(wgConfig);
}

WireGuardConfig GetWireGuardConfig::readWireGuardConfigFromSettings()
{
    QSettings settings;
    if (settings.contains(KEY_WIREGUARD_CONFIG))
    {
        QString s = settings.value(KEY_WIREGUARD_CONFIG, "").toString();
        if (!s.isEmpty())
        {
            QByteArray arr = simpleCrypt_.decryptToByteArray(s);
            QDataStream ds(&arr, QIODevice::ReadOnly);

            quint32 magic, version;
            ds >> magic;
            if (magic == magic_)
            {
                ds >> version;
                if (version <= versionForSerialization_)
                {
                    WireGuardConfig wgConfig;
                    ds >> wgConfig;
                    if (ds.status() == QDataStream::Ok)
                    {
                        return wgConfig;
                    }
                }
            }
        }
    }
    return WireGuardConfig();
}

void GetWireGuardConfig::writeWireGuardConfigToSettings(const WireGuardConfig &wgConfig)
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << wgConfig;
    }
    QSettings settings;
    settings.setValue(KEY_WIREGUARD_CONFIG, simpleCrypt_.encryptToString(arr));
}

void GetWireGuardConfig::removeWireGuardSettings()
{
    QSettings settings;
    settings.remove(KEY_WIREGUARD_CONFIG);

    // remove deprecated values
    // todo remove this code at some point
    settings.remove("wireguardPublicKey");
    settings.remove("wireguardPrivateKey");
    settings.remove("wireguardPresharedKey");
    settings.remove("wireguardAllowedIPs");
}

QString GetWireGuardConfig::generateClientAddress(const QString &cidr)
{
    if (cidr.isEmpty() || wireGuardConfig_.clientPublicKey().isEmpty()) {
        return QString();
    }

    auto slashPos = cidr.indexOf('/');
    if (slashPos == -1) {
        return QString();
    }

    QString ipPart = cidr.left(slashPos);
    QString prefixPart = cidr.mid(slashPos + 1);
    bool ok;
    int prefix = prefixPart.toInt(&ok);
    if (!ok || prefix < 0 || prefix > 32) {
        return QString();
    }

    QStringList octets = ipPart.split('.');
    if (octets.size() != 4) {
        return QString();
    }

    uint32_t networkAddress = 0;
    for (int i = 0; i < 4; i++) {
        int octet = octets[i].toInt(&ok);
        if (!ok || octet < 0 || octet > 255) {
            return QString();
        }
        networkAddress |= (octet << (8 * (3 - i)));
    }

    int hostBits = 32 - prefix;
    uint32_t hostMask = hostBits == 32 ? 0xFFFFFFFF : (1u << hostBits) - 1;
    uint32_t networkMask = ~hostMask;

    QByteArray keyBytes = QByteArray::fromBase64(wireGuardConfig_.clientPublicKey().toLatin1());
    uint32_t hash = wsl::truncatedHash(reinterpret_cast<const uint8_t*>(keyBytes.data()), keyBytes.size());

    uint32_t ipInt = (networkAddress & networkMask) | (hash & hostMask);

    QString result = QString("%1.%2.%3.%4")
        .arg((ipInt >> 24) & 0xFF)
        .arg((ipInt >> 16) & 0xFF)
        .arg((ipInt >> 8) & 0xFF)
        .arg(ipInt & 0xFF);

    return result;
}

QString GetWireGuardConfig::generateClientIpv6Address(const QString &cidr)
{
    if (cidr.isEmpty() || wireGuardConfig_.clientPublicKey().isEmpty()) {
        return QString();
    }

    auto slashPos = cidr.indexOf('/');
    if (slashPos == -1) {
        return QString();
    }

    QString ipPart = cidr.left(slashPos);
    QString prefixPart = cidr.mid(slashPos + 1);
    bool ok;
    int prefix = prefixPart.toInt(&ok);
    if (!ok || prefix < 0 || prefix > 128) {
        return QString();
    }

    QHostAddress networkAddr(ipPart);
    if (networkAddr.protocol() != QAbstractSocket::IPv6Protocol) {
        return QString();
    }

    Q_IPV6ADDR ipv6 = networkAddr.toIPv6Address();

    QByteArray keyBytes = QByteArray::fromBase64(wireGuardConfig_.clientPublicKey().toLatin1());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(keyBytes.constData()), keyBytes.size(), hash);

    for (int i = prefix; i < 128; i++) {
        int byteIdx = i / 8;
        int bitIdx = 7 - (i % 8);
        int hashBit = i - prefix;
        int hashByteIdx = hashBit / 8;
        int hashBitIdx = 7 - (hashBit % 8);

        if (hashByteIdx < SHA256_DIGEST_LENGTH && (hash[hashByteIdx] & (1 << hashBitIdx))) {
            ipv6[byteIdx] |= static_cast<quint8>(1 << bitIdx);
        } else {
            ipv6[byteIdx] &= static_cast<quint8>(~(1 << bitIdx));
        }
    }

    QHostAddress result(ipv6);
    return result.toString();
}
