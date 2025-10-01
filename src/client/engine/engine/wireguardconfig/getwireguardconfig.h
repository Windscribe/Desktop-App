#pragma once

#include <QObject>
#include <wsnet/WSNet.h>
#include "wireguardconfig.h"
#include "utils/simplecrypt.h"

namespace server_api {
class ServerAPI;
}

// kFailoverFailed - this means that all API failovers fail and the API-connection cannot be established.
enum class WireGuardConfigRetCode { kSuccess, kKeyLimit, kFailoverFailed, kFailed };

// manages the logic of getting a WireGuard config using ServerAPI (wgConfigsInit(...) and wgConfigsConnect(...) functions)
// also saves/restores some values of WireGuard config as permanent in settings
// should be used before making connection

class GetWireGuardConfig : public QObject
{
    Q_OBJECT
public:
    GetWireGuardConfig(QObject *parent);
    ~GetWireGuardConfig();

    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId);
    static void removeWireGuardSettings();

signals:
    void getWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config);

private slots:
    void onWgConfigsInitAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onWgConfigsConnectAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);


private:
    static const QString KEY_WIREGUARD_CONFIG;
    WireGuardConfig wireGuardConfig_;
    QString serverName_;
    bool deleteOldestKey_;
    QString deviceId_;
    bool isErrorCode1311Guard_;
    bool isRetryConnectRequest_;
    bool isRetryInitRequest_;
    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;
    SimpleCrypt simpleCrypt_;

    void submitWireguardConnectRequest();
    void submitWireGuardInitRequest(bool generateKeyPair);

    bool getWireGuardKeyPair(QString &publicKey, QString &privateKey);
    void setWireGuardKeyPair(const QString &publicKey, const QString &privateKey);
    bool getWireGuardPeerInfo(QString &presharedKey, QString &allowedIPs);
    void setWireGuardPeerInfo(const QString &presharedKey, const QString &allowedIPs);
    WireGuardConfig readWireGuardConfigFromSettings();
    void writeWireGuardConfigToSettings(const WireGuardConfig &wgConfig);

    // for serialization
    static constexpr quint32 magic_ = 0xFB12A73D;
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed

};
