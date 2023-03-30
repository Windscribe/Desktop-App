#ifndef GETWIREGUARDCONFIG_H
#define GETWIREGUARDCONFIG_H

#include <QObject>
#include "wireguardconfig.h"
#include "utils/simplecrypt.h"
#include "../serverapi/requests/baserequest.h"

namespace server_api {
class ServerAPI;
}

enum class WireGuardConfigRetCode { kSuccess, kKeyLimit, kFailed };

// manages the logic of getting a WireGuard config using ServerAPI (wgConfigsInit(...) and wgConfigsConnect(...) functions)
// also saves/restores some values of WireGuard config as permanent in settings
// should be used before making connection

class GetWireGuardConfig : public QObject
{
    Q_OBJECT
public:
    GetWireGuardConfig(QObject *parent, server_api::ServerAPI *serverAPI);

    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId);
    static void removeWireGuardSettings();

signals:
    void getWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config);

private slots:
    void onWgConfigsInitAnswer();
    void onWgConfigsConnectAnswer();


private:
    static const QString KEY_WIREGUARD_CONFIG;
    server_api::ServerAPI *serverAPI_;
    WireGuardConfig wireGuardConfig_;
    QString serverName_;
    bool deleteOldestKey_;
    QString deviceId_;
    bool isErrorCode1311Guard_;
    bool isRetryConnectRequest_;
    bool isRetryInitRequest_;
    server_api::BaseRequest *request_;
    SimpleCrypt simpleCrypt_;

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

#endif // GETWIREGUARDCONFIG_H
