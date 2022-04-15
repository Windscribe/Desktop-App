#include "getwireguardconfig.h"
#include "engine/serverapi/serverapi.h"
#include "utils/protobuf_includes.h"

const QString GetWireGuardConfig::KEY_WIREGUARD_CONFIG = "wireguardConfig";

GetWireGuardConfig::GetWireGuardConfig(QObject *parent, ServerAPI *serverAPI, uint serverApiUserRole) : QObject(parent), serverAPI_(serverAPI),
    serverApiUserRole_(serverApiUserRole),
    isRequestAlreadyInProgress_(false), simpleCrypt_(0x4572A4ACF31A31BA)
{
    connect(serverAPI_, &ServerAPI::wgConfigsInitAnswer, this, &GetWireGuardConfig::onWgConfigsInitAnswer, Qt::QueuedConnection);
    connect(serverAPI_, &ServerAPI::wgConfigsConnectAnswer, this, &GetWireGuardConfig::onWgConfigsConnectAnswer, Qt::QueuedConnection);
}

void GetWireGuardConfig::getWireGuardConfig(const QString &serverName, bool deleteOldestKey)
{
    if (isRequestAlreadyInProgress_)
    {
        Q_ASSERT(false);
        return;
    }

    isRequestAlreadyInProgress_ = true;
    serverName_ = serverName;
    deleteOldestKey_ = deleteOldestKey;
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
        serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_);
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

    serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_);
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
                serverAPI_->wgConfigsConnect(apiinfo::ApiInfo::getAuthHash(), serverApiUserRole_, true, wireGuardConfig_.clientPublicKey(), serverName_);
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
    ProtoApiInfo::WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    if (!wgConfig.public_key().empty() && !wgConfig.private_key().empty())
    {
        publicKey = QString::fromStdString(wgConfig.public_key());
        privateKey = QString::fromStdString(wgConfig.private_key());
        return true;
    }
    return false;
}

void GetWireGuardConfig::setWireGuardKeyPair(const QString &publicKey, const QString &privateKey)
{
    ProtoApiInfo::WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    wgConfig.set_public_key(publicKey.toStdString());
    wgConfig.set_private_key(privateKey.toStdString());
    writeWireGuardConfigToSettings(wgConfig);
}

bool GetWireGuardConfig::getWireGuardPeerInfo(QString &presharedKey, QString &allowedIPs)
{
    ProtoApiInfo::WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    if (!wgConfig.preshared_key().empty() && !wgConfig.allowed_ips().empty())
    {
        presharedKey = QString::fromStdString(wgConfig.preshared_key());
        allowedIPs = QString::fromStdString(wgConfig.allowed_ips());
        return true;
    }
    return false;
}

void GetWireGuardConfig::setWireGuardPeerInfo(const QString &presharedKey, const QString &allowedIPs)
{
    ProtoApiInfo::WireGuardConfig wgConfig = readWireGuardConfigFromSettings();
    wgConfig.set_preshared_key(presharedKey.toStdString());
    wgConfig.set_allowed_ips(allowedIPs.toStdString());
    writeWireGuardConfigToSettings(wgConfig);
}

ProtoApiInfo::WireGuardConfig GetWireGuardConfig::readWireGuardConfigFromSettings()
{
    QSettings settings;
    if (settings.contains(KEY_WIREGUARD_CONFIG))
    {
        QString s = settings.value(KEY_WIREGUARD_CONFIG, "").toString();
        if (!s.isEmpty())
        {
            QByteArray arr = simpleCrypt_.decryptToByteArray(s);
            ProtoApiInfo::WireGuardConfig wgConfig;
            if (wgConfig.ParseFromArray(arr.data(), arr.size()))
            {
                return wgConfig;
            }
        }
    }
    return ProtoApiInfo::WireGuardConfig();
}

void GetWireGuardConfig::writeWireGuardConfigToSettings(const ProtoApiInfo::WireGuardConfig &wgConfig)
{
    QSettings settings;
    size_t size = wgConfig.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    wgConfig.SerializeToArray(arr.data(), size);
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
