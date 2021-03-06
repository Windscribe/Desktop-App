#ifndef GETWIREGUARDCONFIG_H
#define GETWIREGUARDCONFIG_H

#include <QObject>
#include "engine/types/types.h"
#include "wireguardconfig.h"
#include "utils/simplecrypt.h"

class ServerAPI;

// manages the logic of getting a WireGuard config using ServerAPI (wgConfigsInit(...) and wgConfigsConnect(...) functions)
// also saves/restores some values of WireGuard config as permanent in settings
// should be used before making connection

class GetWireGuardConfig : public QObject
{
    Q_OBJECT
public:
    GetWireGuardConfig(QObject *parent, ServerAPI *serverAPI, uint serverApiUserRole);

    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey);
    static void removeWireGuardSettings();

signals:
    void getWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config);

private slots:
    void onWgConfigsInitAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &presharedKey, const QString &allowedIps);
    void onWgConfigsConnectAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &ipAddress, const QString &dnsAddress);


private:
    static const QString KEY_WIREGUARD_CONFIG;
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    WireGuardConfig wireGuardConfig_;
    QString serverName_;
    bool deleteOldestKey_;
    bool isErrorCode1311Guard_;
    bool isRetryConnectRequest_;
    bool isRetryInitRequest_;
    bool isRequestAlreadyInProgress_;
    SimpleCrypt simpleCrypt_;

    void submitWireGuardInitRequest(bool generateKeyPair);

    bool getWireGuardKeyPair(QString &publicKey, QString &privateKey);
    void setWireGuardKeyPair(const QString &publicKey, const QString &privateKey);
    bool getWireGuardPeerInfo(QString &presharedKey, QString &allowedIPs);
    void setWireGuardPeerInfo(const QString &presharedKey, const QString &allowedIPs);
    ProtoApiInfo::WireGuardConfig readWireGuardConfigFromSettings();
    void writeWireGuardConfigToSettings(const ProtoApiInfo::WireGuardConfig &wgConfig);
};

#endif // GETWIREGUARDCONFIG_H
