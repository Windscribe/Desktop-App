#include "getwireguardconfig.h"
#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/apiinfo.h"
#include "types/global_consts.h"
#include "utils/ws_assert.h"
#include "engine/serverapi/requests/wgconfigsinitrequest.h"
#include "engine/serverapi/requests/wgconfigsconnectrequest.h"

extern "C" {
    #include "legacy_protobuf_support/apiinfo.pb-c.h"

#include <QDataStream>
}

const QString GetWireGuardConfig::KEY_WIREGUARD_CONFIG = "wireguardConfig";

GetWireGuardConfig::GetWireGuardConfig(QObject *parent, server_api::ServerAPI *serverAPI) : QObject(parent), serverAPI_(serverAPI),
    isRequestAlreadyInProgress_(false), simpleCrypt_(SIMPLE_CRYPT_KEY)
{
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
        server_api::BaseRequest *requestConfigsConnect = serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
        connect(requestConfigsConnect, &server_api::BaseRequest::finished, this, &GetWireGuardConfig::onWgConfigsConnectAnswer);
    }
    else
    {
        submitWireGuardInitRequest(true);
    }
}

void GetWireGuardConfig::onWgConfigsInitAnswer()
{
    QSharedPointer<server_api::WgConfigsInitRequest> request(static_cast<server_api::WgConfigsInitRequest *>(sender()), &QObject::deleteLater);

    if (request->networkRetCode() != SERVER_RETURN_SUCCESS) {
        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    if (request->isErrorCode())
    {
        WireGuardConfigRetCode newRetCode = WireGuardConfigRetCode::kFailed;

        if (request->errorCode() == 1310) {
            // This error indicates the server was unable to generate the preshared key.
            // Retry the init request one time, then abort the WireGuard connection attempt.
            if (!isRetryInitRequest_) {
                isRetryInitRequest_ = true;
                submitWireGuardInitRequest(false);
                return;
            }
         }
         else if (request->errorCode() == 1313) {
            // This error indicates the user has used up all of their public key slots on the server.
            // Ask them if they want to delete their oldest registered key and try again.
            newRetCode = WireGuardConfigRetCode::kKeyLimit;
         }

        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(newRetCode, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setPeerPresharedKey(request->presharedKey());
    wireGuardConfig_.setPeerAllowedIPs(WireGuardConfig::stripIpv6Address(request->allowedIps()));

    // Persist the peer parameters we received.
    setWireGuardPeerInfo(wireGuardConfig_.peerPresharedKey(), wireGuardConfig_.peerAllowedIps());

    server_api::BaseRequest *requestConfigsConnect = serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
    connect(requestConfigsConnect, &server_api::BaseRequest::finished, this, &GetWireGuardConfig::onWgConfigsConnectAnswer);

}

void GetWireGuardConfig::onWgConfigsConnectAnswer()
{
    QSharedPointer<server_api::WgConfigsConnectRequest> request(static_cast<server_api::WgConfigsConnectRequest *>(sender()), &QObject::deleteLater);

    if (request->networkRetCode() != SERVER_RETURN_SUCCESS)
    {
        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    if (request->isErrorCode())
    {
        if (request->errorCode() == 1311) {
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
        else if (request->errorCode() == 1312) {
            // This error is returned when an interface address cannot be assigned. This is likely a major API issue,
            // since this shouldn't happen. Retry the 'connect' API once. If it fails again, abort the connection attempt.
            if (!isRetryConnectRequest_) {
                isRetryConnectRequest_ = true;
                server_api::BaseRequest *request = serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), wireGuardConfig_.clientPublicKey(), serverName_, deviceId_);
                connect(request, &server_api::BaseRequest::finished, this, &GetWireGuardConfig::onWgConfigsConnectAnswer);
                return;
            }
        }

        isRequestAlreadyInProgress_ = false;
        emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
        return;
    }

    wireGuardConfig_.setClientIpAddress(WireGuardConfig::stripIpv6Address(request->ipAddress()));
    wireGuardConfig_.setClientDnsAddress(WireGuardConfig::stripIpv6Address(request->dnsAddress()));
    isRequestAlreadyInProgress_ = false;
    emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kSuccess, wireGuardConfig_);
}

void GetWireGuardConfig::submitWireGuardInitRequest(bool generateKeyPair)
{
    if (generateKeyPair)
    {
        if (!wireGuardConfig_.generateKeyPair())
        {
            isRequestAlreadyInProgress_ = false;
            emit getWireGuardConfigAnswer(WireGuardConfigRetCode::kFailed, wireGuardConfig_);
            return;
        }
        // Persist the key-pair we're about to register with the server.
        setWireGuardKeyPair(wireGuardConfig_.clientPublicKey(), wireGuardConfig_.clientPrivateKey());
    }
    server_api::BaseRequest *request = serverAPI_->wgConfigsInit(apiinfo::ApiInfo::getAuthHash(), wireGuardConfig_.clientPublicKey(), deleteOldestKey_);
    connect(request, &server_api::BaseRequest::finished, this, &GetWireGuardConfig::onWgConfigsInitAnswer);
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
