#pragma once

#include <QObject>
#include "backend/backend.h"
#include "blockconnect.h"
#include "ipc/clicommands.h"
#include "localipcserver/localipcserver.h"
#include "multipleaccountdetection/imultipleaccountdetection.h"
#include "types/enums.h"
#include "types/protocol.h"

class MainService : public QObject
{
    Q_OBJECT

public:
    MainService();
    ~MainService();

    void stop();

signals:
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);

private slots:
    void onBackendCheckUpdateChanged(const api_responses::CheckUpdate &info);
    void onBackendInitFinished(INIT_STATE initState);
    void onBackendSessionStatusChanged(const api_responses::SessionStatus &sessionStatus);
    void onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error);
    void onBackendWireGuardAtKeyLimit();
    void onBackendLocalDnsServerNotAvailable();
    void onBackendMyIpChanged(const QString &ip, bool isFromDisconnectedState);

    void onPreferencesAllowLanTrafficChanged(bool allowLanTraffic);
    void onPreferencesFirewallSettingsChanged(const types::FirewallSettings &fm);
    void onPreferencesLaunchOnStartupChanged(bool enabled);
    void onPreferencesShareProxyGatewayChanged(const types::ShareProxyGateway &sp);
    void onPreferencesSplitTunnelingChanged(const types::SplitTunneling &st);

    // From CLI commands
    void onConnectToLocation(const LocationID &id, const types::Protocol &protocol, uint port);
    void onConnectToStaticIpLocation(const QString &location, const types::Protocol &protocol, uint port);
    void onLogin(const QString &username, const QString &password, const QString &code2fa);
    void onShowLocations(IPC::CliCommands::LocationType type);
    void onSetKeyLimitBehavior(bool deleteKey);
    void onPinIp();
    void onUnpinIp(const QString &ip);

private:
    Backend *backend_;
    LocalIPCServer *localIpcServer_;

    BlockConnect blockConnect_;
    QSharedPointer<IMultipleAccountDetection> multipleAccountDetection_;

    bool isExitingAfterUpdate_;
    bool keyLimitDelete_;

    void setInitialFirewallState();

    QString connectedIp_;
};
