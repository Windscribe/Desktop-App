#ifndef GETWIREGUARDCONFIG_H
#define GETWIREGUARDCONFIG_H

#include <QObject>
#include "engine/types/types.h"
#include "wireguardconfig.h"

class ServerAPI;

// manages the logic of getting a WireGuard config using ServerAPI (wgConfigsInit(...) and wgConfigsConnect(...) functions)
// also saves/restores some values of WireGuard config as permanent in settings
// should be used before making connection

class GetWireGuardConfig : public QObject
{
    Q_OBJECT
public:
    GetWireGuardConfig(QObject *parent, ServerAPI *serverAPI);

    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey);

signals:
    void getWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config);

private slots:
    void onWgConfigsInitAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &presharedKey, const QString &allowedIps);
    void onWgConfigsConnectAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &ipAddress, const QString &dnsAddress);


private:
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    WireGuardConfig wireGuardConfig_;
    QString serverName_;
    bool deleteOldestKey_;
    bool isErrorCode1311Guard_;
    bool isRetryConnectRequest_;
    bool isRetryInitRequest_;
    bool isRequestAlreadyInProgress_;

    void submitWireGuardInitRequest(bool generateKeyPair);
};

#endif // GETWIREGUARDCONFIG_H
