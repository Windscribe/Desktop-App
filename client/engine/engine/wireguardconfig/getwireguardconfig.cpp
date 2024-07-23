#include "getwireguardconfig.h"
#include <QSettings>
#include <QTimer>
#include "types/global_consts.h"
#include "api_responses/wgconfigs_connect.h"
#include "api_responses/wgconfigs_init.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

extern "C" {
    #include "legacy_protobuf_support/apiinfo.pb-c.h"

#include <QDataStream>
}

const QString GetWireGuardConfig::KEY_WIREGUARD_CONFIG = "wireguardConfig";

using namespace wsnet;

GetWireGuardConfig::GetWireGuardConfig(QObject *parent) : QObject(parent),
    simpleCrypt_(SIMPLE_CRYPT_KEY)
{
}

GetWireGuardConfig::~GetWireGuardConfig()
{
    SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(request_);
}

void GetWireGuardConfig::getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId)
{
    WS_ASSERT(request_ == nullptr);

    serverName_ = serverName;
    deleteOldestKey_ = deleteOldestKey;
    deviceId_ = deviceId;
    isErrorCode1311Guard_ = false;
    isRetryConnectRequest_ = false;
    isRetryInitRequest_ = false;

    wireGuardConfig_.reset();
    // restore a key-pair and peer parameters stored on disk
    // if they are not found in settings, they will be generated and saved later by this class flow.
    QString publicKey, privateKey, presharedKey, allowedIPs;
    if (getWireGuardKeyPair(publicKey, privateKey) && getWireGuardPeerInfo(presharedKey, allowedIPs)) {
        wireGuardConfig_.setKeyPair(publicKey, privateKey);
        wireGuardConfig_.setPeerPresharedKey(presharedKey);
        wireGuardConfig_.setPeerAllowedIPs(allowedIPs);
        submitWireguardConnectRequest();
    } else {
        submitWireGuardInitRequest(true);
    }
}

void GetWireGuardConfig::onWgConfigsInitAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData)
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

        emit getWireGuardConfigAnswer(newRetCode, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setPeerPresharedKey(res.presharedKey());
    wireGuardConfig_.setPeerAllowedIPs(WireGuardConfig::stripIpv6Address(res.allowedIps()));

    // Persist the peer parameters we received.
    setWireGuardPeerInfo(wireGuardConfig_.peerPresharedKey(), wireGuardConfig_.peerAllowedIps());

    submitWireguardConnectRequest();
}

void GetWireGuardConfig::onWgConfigsConnectAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    request_.reset();

    if (serverApiRetCode == ServerApiRetCode::kFailoverFailed) {
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailoverFailed, wireGuardConfig_);
        return;
    } else if (serverApiRetCode != ServerApiRetCode::kSuccess) {
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    api_responses::WgConfigsConnect res(jsonData);
    if (res.isErrorCode()) {
        if (res.errorCode() == 1311) {
            // This means all the user's public keys were nuked on the server and what we have locally is useless.
            // Discard all locally stored keys and start fresh with a new 'init' API call.  In case of an unexpected
            // API issue, guard against looping behavior where you run "init" and "connect" which returns the same error.
            if (!isErrorCode1311Guard_) {
                isErrorCode1311Guard_ = true;
                wireGuardConfig_.reset();
                removeWireGuardSettings();
                submitWireGuardInitRequest(true);
                return;
            }
        } else if (res.errorCode() == 1312) {
            // This error is returned when an interface address cannot be assigned. This is likely a major API issue,
            // since this shouldn't happen. Retry the 'connect' API once. If it fails again, abort the connection attempt.
            if (!isRetryConnectRequest_) {
                isRetryConnectRequest_ = true;
                submitWireguardConnectRequest();
                return;
            }
        }

        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setClientIpAddress(WireGuardConfig::stripIpv6Address(res.ipAddress()));
    wireGuardConfig_.setClientDnsAddress(WireGuardConfig::stripIpv6Address(res.dnsAddress()));
    emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, wireGuardConfig_);
}

void GetWireGuardConfig::submitWireguardConnectRequest()
{
    WS_ASSERT(request_ == nullptr);
    request_ = WSNet::instance()->serverAPI()->wgConfigsConnect(WSNet::instance()->apiResourcersManager()->authHash(), wireGuardConfig_.clientPublicKey().toStdString(),
                                                                serverName_.toStdString(), deviceId_.toStdString(), std::string(),
                                                                [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
                                                                {
                                                                    QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData]() {
                                                                        onWgConfigsConnectAnswer(serverApiRetCode, jsonData);
                                                                    });
                                                                });
}

void GetWireGuardConfig::submitWireGuardInitRequest(bool generateKeyPair)
{
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
                                                                    QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData]() {
                                                                        onWgConfigsInitAnswer(serverApiRetCode, jsonData);
                                                                    });
                                                                });
}

bool GetWireGuardConfig::getWireGuardKeyPair(QString &publicKey, QString &privateKey)
{
    WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    if (!wgConfig.clientPublicKey().isEmpty() && !wgConfig.clientPrivateKey().isEmpty())
    {
        publicKey = wgConfig.clientPublicKey();
        privateKey = wgConfig.clientPrivateKey();
        return true;
    }
    return false;
}

void GetWireGuardConfig::setWireGuardKeyPair(const QString &publicKey, const QString &privateKey)
{
    WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    wgConfig.setKeyPair(publicKey, privateKey);
    writeWireGuardConfigToSettings(wgConfig);
}

bool GetWireGuardConfig::getWireGuardPeerInfo(QString &presharedKey, QString &allowedIPs)
{
    WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    if (!wgConfig.peerPresharedKey().isEmpty() && !wgConfig.peerAllowedIps().isEmpty())
    {
        presharedKey = wgConfig.peerPresharedKey();
        allowedIPs = wgConfig.peerAllowedIps();
        return true;
    }
    return false;
}

void GetWireGuardConfig::setWireGuardPeerInfo(const QString &presharedKey, const QString &allowedIPs)
{
    WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    wgConfig.setPeerPresharedKey(presharedKey);
    wgConfig.setPeerAllowedIPs(allowedIPs);
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

            WireGuardConfig wgConfig;
            {
                SimpleCrypt simpleCryptLegacy(0x4572A4ACF31A31BA);
                QByteArray arr = simpleCryptLegacy.decryptToByteArray(s);
                // try load from legacy protobuf
                // todo remove this code at some point later
                ProtoApiInfo__WireGuardConfig *wgc = proto_api_info__wire_guard_config__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
                if (wgc)
                {
                    if (wgc->public_key && wgc->private_key)
                        wgConfig.setKeyPair(QString::fromStdString(wgc->public_key), QString::fromStdString(wgc->private_key));

                    if (wgc->preshared_key)
                        wgConfig.setPeerPresharedKey(QString::fromStdString(wgc->preshared_key));

                    if (wgc->allowed_ips)
                        wgConfig.setPeerAllowedIPs(QString::fromStdString(wgc->allowed_ips));

                    proto_api_info__wire_guard_config__free_unpacked(wgc, NULL);
                }
            }

            return wgConfig;
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
