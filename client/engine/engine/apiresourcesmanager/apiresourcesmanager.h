#pragma once

#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/apiinfo.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/serverapi/requests/sessionerrorcode.h"
#include "types/notification.h"
#include "types/checkupdate.h"

namespace api_resources_manager {

enum class RequestType { kSessionStatus, kLocations, kServerCredentialsOpenVPN, kServerCredentialsIkev2, kServerConfigs, kPortMap, kStaticIps, kNotifications, kCheckUpdate };

// Manages getting and updating resources from ServerAPI according to Resource Re-fetch Schedule
// https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover#resource-re-fetch-schedule
class ApiResourcesManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiResourcesManager(QObject *parent, server_api::ServerAPI *serverAPI, IConnectStateController *connectStateController);
    virtual ~ApiResourcesManager();

    void fetchAllWithAuthHash();
    void login(const QString &username, const QString &password, const QString &code2fa);
    void fetchSessionOnForegroundEvent();
    void checkUpdate(UPDATE_CHANNEL channel);

    bool isReadyForLogin() const;

signals:
    void readyForLogin();
    void loginFailed(LOGIN_RET loginRetCode, const QString &errorMessage);

    void sessionDeleted();
    void sessionUpdated(const types::SessionStatus &sessionStatus);
    void locationsUpdated(const QVector<apiinfo::Location> &locations);
    void notificationsUpdated(const QVector<types::Notification> &notifications);
    void staticIpsUpdated(const apiinfo::StaticIps &staticIps);
    void checkUpdateUpdated(const types::CheckUpdate &checkUpdate);

private slots:
    void onInitialSessionAnswer();
    void onLoginAnswer();
    void onServerConfigsAnswer();
    void onServerCredentialsOpenVpnAnswer();
    void onServerCredentialsIkev2Answer();
    void onServerLocationsAnswer();
    void onPortMapAnswer();
    void onStaticIpsAnswer();
    void onNotificationsAnswer();
    void onSessionAnswer();
    void onCheckUpdateAnswer();

    void onFetchTimer();

private:
    server_api::ServerAPI *serverAPI_;
    IConnectStateController *connectStateController_;
    apiinfo::ApiInfo apiInfo_;

    UPDATE_CHANNEL updateChannel_;
    bool isUpdateChannelInit_ = false;

    static constexpr int kMinute = 60 * 1000;
    static constexpr int kHour = 60 * 60 * 1000;
    static constexpr int k24Hours = 24 * 60 * 60 * 1000;

    QHash<RequestType, qint64> lastUpdateTimeMs_;
    QHash<RequestType, server_api::BaseRequest *> requestsInProgress_;
    QTimer *fetchTimer_;

    types::SessionStatus prevSessionStatus_;
    types::SessionStatus prevSessionForLogging_;

    // return true if need retry request
    void handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, server_api::SessionErrorCode sessionErrorCode, const types::SessionStatus &sessionStatus,
                                                     const QString &authHash, const QString &errorMessage);

    void checkForReadyLogin();
    void fetchAll(const QString &authHash);

    void fetchServerConfigs(const QString &authHash);
    void fetchServerCredentialsOpenVpn(const QString &authHash);
    void fetchServerCredentialsIkev2(const QString &authHash);
    void fetchLocations();
    void fetchPortMap(const QString &authHash);
    void fetchStaticIps(const QString &authHash);
    void fetchNotifications(const QString &authHash);
    void fetchSession(const QString &authHash);
    void fetchCheckUpdate();

    void updateSessionStatus();

};

} // namespace api_resources_manager
