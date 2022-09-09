#include "getwireguardconfig.h"
#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/apiinfo.h"
#include "types/global_consts.h"
#include "utils/ws_assert.h"

extern "C" {
    #include "legacy_protobuf_support/apiinfo.pb-c.h"

#include <QDataStream>
}

const QString GetWireGuardConfig::KEY_WIREGUARD_CONFIG = "wireguardConfig";

GetWireGuardConfig::GetWireGuardConfig(QObject *parent, ServerAPI *serverAPI, uint serverApiUserRole) : QObject(parent), serverAPI_(serverAPI),
    serverApiUserRole_(serverApiUserRole),
    isRequestAlreadyInProgress_(false), simpleCrypt_(SIMPLE_CRYPT_KEY)
{
    connect(serverAPI_, &ServerAPI::wgConfigsInitAnswer, this, &GetWireGuardConfig::onWgConfigsInitAnswer, Qt::QueuedConnection);
    connect(serverAPI_, &ServerAPI::wgConfigsConnectAnswer, this, &GetWireGuardConfig::onWgConfigsConnectAnswer, Qt::QueuedConnection);
}

void GetWireGuardConfig::getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId)
{
    if (isRequestAlreadyInProgress_)
    {
        WS_ASSERT(false);
        return;
    }

    isRequestAlreadyInProgress_ = true;
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
    if (getWireGuardKeyPair(publicKey, privateKey) && getWireGuardPeerInfo(presharedKey, allowedIPs))
    {
        wireGuardConfig_.setKeyPair(publicKey, privateKey);
        wireGuardConfig_.setPeerPresharedKey(presharedKey);
        wireGuardConfig_.setPeerAllowedIPs(allowedIPs);
        serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
    }
    else
    {
        submitWireGuardInitRequest(true);
    }
}

void GetWireGuardConfig::onWgConfigsInitAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &presharedKey, const QString &allowedIps)
{
    if (serverApiUserRole_ != userRole)
    {
        return;
    }

    if (retCode != SERVER_RETURN_SUCCESS)
    {
        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(retCode, wireGuardConfig_);
        return;
    }

    if (isErrorCode)
    {
        SERVER_API_RET_CODE newRetCode = SERVER_RETURN_NETWORK_ERROR;

        if (errorCode == 1310) {
            // This error indicates the server was unable to generate the preshared key.
            // Retry the init request one time, then abort the WireGuard connection attempt.
            if (!isRetryInitRequest_) {
                isRetryInitRequest_ = true;
                submitWireGuardInitRequest(false);
                return;
            }
         }
         else if (errorCode == 1313) {
            // This error indicates the user has used up all of their public key slots on the server.
            // Ask them if they want to delete their oldest registered key and try again.
            newRetCode = SERVER_RETURN_WIREGUARD_KEY_LIMIT;
         }

        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(newRetCode, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setPeerPresharedKey(presharedKey);
    wireGuardConfig_.setPeerAllowedIPs(WireGuardConfig::stripIpv6Address(allowedIps));

    // Persist the peer parameters we received.
    setWireGuardPeerInfo(wireGuardConfig_.peerPresharedKey(), wireGuardConfig_.peerAllowedIps());

    serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
}

void GetWireGuardConfig::onWgConfigsConnectAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &ipAddress, const QString &dnsAddress)
{
    if (serverApiUserRole_ != userRole)
    {
        return;
    }

    if (retCode != SERVER_RETURN_SUCCESS)
    {
        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(retCode, wireGuardConfig_);
        return;
    }

    if (isErrorCode)
    {
        if (errorCode == 1311) {
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
        }
        else if (errorCode == 1312) {
            // This error is returned when an interface address cannot be assigned. This is likely a major API issue,
            // since this shouldn't happen. Retry the 'connect' API once. If it fails again, abort the connection attempt.
            if (!isRetryConnectRequest_) {
                isRetryConnectRequest_ = true;
                serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
                return;
            }
        }

        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(SERVER_RETURN_NETWORK_ERROR, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setClientIpAddress(WireGuardConfig::stripIpv6Address(ipAddress));
    wireGuardConfig_.setClientDnsAddress(WireGuardConfig::stripIpv6Address(dnsAddress));
    isRequestAlreadyInProgress_ = false;
    emit getWireGuardConfigAnswer(SERVER_RETURN_SUCCESS, wireGuardConfig_);
}

void GetWireGuardConfig::submitWireGuardInitRequest(bool generateKeyPair)
{
    if (generateKeyPair)
    {
        if (!wireGuardConfig_.generateKeyPair())
        {
            isRequestAlreadyInProgress_ = false;
            emit getWireGuardConfigAnswer(SERVER_RETURN_NETWORK_ERROR, wireGuardConfig_);
            return;
        }
        // Persist the key-pair we're about to register with the server.
        setWireGuardKeyPair(wireGuardConfig_.clientPublicKey(), wireGuardConfig_.clientPrivateKey());
    }
    serverAPI_->wgConfigsInit(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), deleteOldestKey_);
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
                    wgConfig.setKeyPair(QString::fromStdString(wgc->public_key), QString::fromStdString(wgc->private_key));
                    wgConfig.setPeerPresharedKey(QString::fromStdString(wgc->preshared_key));
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
