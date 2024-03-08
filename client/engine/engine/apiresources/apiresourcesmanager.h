#pragma once

#include "engine/apiinfo/apiinfo.h"
#include <wsnet/WSNet.h>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkdetectionmanager/waitfornetworkconnectivity.h"
#include "api_responses/notification.h"

namespace api_resources {

enum class RequestType { kSessionStatus, kLocations, kServerCredentialsOpenVPN, kServerCredentialsIkev2, kServerConfigs, kPortMap, kStaticIps, kNotifications };

// Manages getting and updating resources from ServerAPI according to Resource Re-fetch Schedule
// https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover#resource-re-fetch-schedule
class ApiResourcesManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiResourcesManager(QObject *parent,IConnectStateController *connectStateController, INetworkDetectionManager *networkDetectionManager);
    virtual ~ApiResourcesManager();

    // one of these functions should be called only once for the lifetime of the object
    void fetchAllWithAuthHash();
    void login(const QString &username, const QString &password, const QString &code2fa);

    void fetchSession();

    // start fetching the server credentials and openvpn config
    // after they are fetched the signal serverCredentialsFetched() is emitted
    void fetchServerCredentials();

    bool loadFromSettings();

    // true if the session request was successful at least once
    bool isLoggedIn() const;

    // in order to install server credentials from outside the class (from RefetchServerCredentials)
    void setServerCredentials(const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig);

    static bool isAuthHashExists() { return !apiinfo::ApiInfo::getAuthHash().isEmpty(); }
    static QString authHash() { return apiinfo::ApiInfo::getAuthHash(); }
    static bool isCanBeLoadFromSettings();
    static void removeFromSettings() { apiinfo::ApiInfo::removeFromSettings(); }

    api_responses::SessionStatus sessionStatus() const { return apiInfo_.getSessionStatus(); }
    api_responses::PortMap portMap() const { return apiInfo_.getPortMap(); }
    QVector<api_responses::Location> locations() const { return apiInfo_.getLocations(); }
    QStringList forceDisconnectNodes() const { return apiInfo_.getForceDisconnectNodes(); }
    api_responses::StaticIps staticIps() const { return apiInfo_.getStaticIps(); }
    apiinfo::ServerCredentials serverCredentials() const { return apiInfo_.getServerCredentials(); }
    QString ovpnConfig() const { return apiInfo_.getOvpnConfig(); }

signals:
    void readyForLogin();
    void loginFailed(LOGIN_RET loginRetCode, const QString &errorMessage);

    void sessionDeleted();
    void sessionUpdated(const api_responses::SessionStatus &sessionStatus);
    // countryOverride parameter from the response or empty if there was no this parameter
    void locationsUpdated(const QString &countryOverride);
    void staticIpsUpdated();
    void notificationsUpdated(const QVector<api_responses::Notification> &notifications);

    void serverCredentialsFetched();

private slots:
    void onFetchTimer();
    void onConnectivityOnline();
    void onConnectivityTimeoutExpired();

private:
    IConnectStateController *connectStateController_;
    WaitForNetworkConnectivity *waitForNetworkConnectivity_;
    apiinfo::ApiInfo apiInfo_;

    static constexpr int kMinute = 60 * 1000;
    static constexpr int kHour = 60 * 60 * 1000;
    static constexpr int k24Hours = 24 * 60 * 60 * 1000;

    QHash<RequestType, qint64> lastUpdateTimeMs_;
    QHash<RequestType, std::shared_ptr<wsnet::WSNetCancelableCallback> > requestsInProgress_;
    QTimer *fetchTimer_;

    api_responses::SessionStatus prevSessionStatus_;
    api_responses::SessionStatus prevSessionForLogging_;

    // internal variables for fetchServerCredentials() functionality
    bool isFetchingServerCredentials_ = false;
    bool isOpenVpnCredentialsReceived_;
    bool isIkev2CredentialsReceived_;
    bool isServerConfigsReceived_;

    static constexpr int kWaitTimeForNoNetwork = 10000;

    void handleLoginOrSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);

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

    void onInitialSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onLoginAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData,
                       const QString &username, const QString &password, const QString &code2fa);
    void onSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerConfigsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerCredentialsOpenVpnAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerCredentialsIkev2Answer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onServerLocationsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onPortMapAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onStaticIpsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
    void onNotificationsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);

};

} // namespace api_resources
