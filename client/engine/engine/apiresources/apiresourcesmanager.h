#pragma once

#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/apiinfo.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/serverapi/requests/sessionerrorcode.h"
#include "engine/networkdetectionmanager/waitfornetworkconnectivity.h"
#include "types/notification.h"

namespace api_resources {

enum class RequestType { kSessionStatus, kLocations, kServerCredentialsOpenVPN, kServerCredentialsIkev2, kServerConfigs, kPortMap, kStaticIps, kNotifications };

// Manages getting and updating resources from ServerAPI according to Resource Re-fetch Schedule
// https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover#resource-re-fetch-schedule
class ApiResourcesManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiResourcesManager(QObject *parent, server_api::ServerAPI *serverAPI, IConnectStateController *connectStateController, INetworkDetectionManager *networkDetectionManager);
    virtual ~ApiResourcesManager();

    // one of these functions should be called only once for the lifetime of the object
    void fetchAllWithAuthHash();
    void login(const QString &username, const QString &password, const QString &code2fa);

    void signOut();
    void fetchSession();

    // start fetching the server credentials and openvpn config
    // after they are fetched the signal serverCredentialsFetched() is emitted
    void fetchServerCredentials();

    bool loadFromSettings();

    // in order to install server credentials from outside the class (from RefetchServerCredentials)
    void setServerCredentials(const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig);

    static bool isAuthHashExists() { return !apiinfo::ApiInfo::getAuthHash().isEmpty(); }
    static QString authHash() { return apiinfo::ApiInfo::getAuthHash(); }
    static bool isCanBeLoadFromSettings();
    static void removeFromSettings() { apiinfo::ApiInfo::removeFromSettings(); }

    types::SessionStatus sessionStatus() const { return apiInfo_.getSessionStatus(); }
    types::PortMap portMap() const { return apiInfo_.getPortMap(); }
    QVector<apiinfo::Location> locations() const { return apiInfo_.getLocations(); }
    QStringList forceDisconnectNodes() const { return apiInfo_.getForceDisconnectNodes(); }
    apiinfo::StaticIps staticIps() const { return apiInfo_.getStaticIps(); }
    apiinfo::ServerCredentials serverCredentials() const { return apiInfo_.getServerCredentials(); }
    QString ovpnConfig() const { return apiInfo_.getOvpnConfig(); }

signals:
    void readyForLogin();
    void loginFailed(LOGIN_RET loginRetCode, const QString &errorMessage);

    void sessionDeleted();
    void sessionUpdated(const types::SessionStatus &sessionStatus);
    void locationsUpdated();
    void staticIpsUpdated();
    void notificationsUpdated(const QVector<types::Notification> &notifications);

    void serverCredentialsFetched();

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

    void onFetchTimer();

    void onConnectivityOnline();
    void onConnectivityTimeoutExpired();

private:
    server_api::ServerAPI *serverAPI_;
    IConnectStateController *connectStateController_;
    WaitForNetworkConnectivity *waitForNetworkConnectivity_;
    apiinfo::ApiInfo apiInfo_;

    static constexpr int kMinute = 60 * 1000;
    static constexpr int kHour = 60 * 60 * 1000;
    static constexpr int k24Hours = 24 * 60 * 60 * 1000;

    QHash<RequestType, qint64> lastUpdateTimeMs_;
    QHash<RequestType, QPointer<server_api::BaseRequest>> requestsInProgress_;
    QTimer *fetchTimer_;

    types::SessionStatus prevSessionStatus_;
    types::SessionStatus prevSessionForLogging_;

    // internal variables for fetchServerCredentials() functionality
    bool isFetchingServerCredentials_ = false;
    bool isOpenVpnCredentialsReceived_;
    bool isIkev2CredentialsReceived_;
    bool isServerConfigsReceived_;

    static constexpr int kWaitTimeForNoNetwork = 10000;

    void handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, server_api::SessionErrorCode sessionErrorCode, const types::SessionStatus &sessionStatus,
                                                     const QString &authHash, const QString &errorMessage);

    void checkForReadyLogin();
    void checkForServerCredentialsFetchFinished();

    void fetchAllWithAuthHashImpl();
    void loginImpl(const QString &username, const QString &password, const QString &code2fa);

    void fetchAll(const QString &authHash);
    void fetchServerConfigs(const QString &authHash);
    void fetchServerCredentialsOpenVpn(const QString &authHash);
    void fetchServerCredentialsIkev2(const QString &authHash);
    void fetchLocations();
    void fetchPortMap(const QString &authHash);
    void fetchStaticIps(const QString &authHash);
    void fetchNotifications(const QString &authHash);
    void fetchSession(const QString &authHash);

    void updateSessionStatus();

    void saveApiInfoToSettings();
};

} // namespace api_resources
