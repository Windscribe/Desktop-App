#pragma once

#include <QObject>

#include "types/notification.h"
#include "types/proxysettings.h"
#include "types/robertfilter.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "requests/baserequest.h"


class IConnectStateController;

namespace server_api {

/*
Access to API endpoint.
Example of a typical usage in the calling code
   server_api::BaseRequest *request = serverAPI_->login(username, password, code2fa);
   connect(request, &server_api::BaseRequest::finished, this, &LoginController::onLoginAnswer);
   .....
  void LoginController::onLoginAnswer()
{
    QSharedPointer<server_api::LoginRequest> request(static_cast<server_api::LoginRequest *>(sender()), &QObject::deleteLater);
    ... some code processing request ...;
}
that is, the calling code should take care of the deletion returned server_api::BaseRequest object (preferably via deleteLater())
*/

class ServerAPI : public QObject
{
    Q_OBJECT
public:
    explicit ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);
    virtual ~ServerAPI();

    uint getAvailableUserRole();

    // FIXME: move to NetworkAccessManager
    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    // if true, then all requests works
    // if false, then only request for SERVER_API_ROLE_LOGIN_CONTROLLER and SERVER_API_ROLE_ACCESS_IPS_CONTROLLER roles works
    void setRequestsEnabled(bool bEnable);
    bool isRequestsEnabled() const;

    // set single hostname for make API requests
    void setHostname(const QString &hostname);
    QString getHostname() const;

    BaseRequest *accessIps(const QString &hostIp);
    BaseRequest *login(const QString &username, const QString &password, const QString &code2fa);
    BaseRequest *session(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *serverLocations(const QString &language, bool isNeedCheckRequestsEnabled,
                         const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList);
    BaseRequest *serverCredentials(const QString &authHash, PROTOCOL protocol, bool isNeedCheckRequestsEnabled);
    BaseRequest *deleteSession(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *serverConfigs(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *portMap(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *recordInstall(bool isNeedCheckRequestsEnabled);
    BaseRequest *confirmEmail(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *webSession(const QString authHash, WEB_SESSION_PURPOSE purpose, bool isNeedCheckRequestsEnabled);

    BaseRequest *myIP(int timeout, bool isNeedCheckRequestsEnabled);

    BaseRequest *checkUpdate(UPDATE_CHANNEL updateChannel, bool isNeedCheckRequestsEnabled);
    BaseRequest *debugLog(const QString &username, const QString &strLog, bool isNeedCheckRequestsEnabled);
    BaseRequest *speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, bool isNeedCheckRequestsEnabled);

    BaseRequest *staticIps(const QString &authHash, const QString &deviceId, bool isNeedCheckRequestsEnabled);

    void pingTest(quint64 cmdId, uint timeout, bool bWriteLog);
    void cancelPingTest(quint64 cmdId);

    void notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);

    void getRobertFilters(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void setRobertFilter(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter);

    void wgConfigsInit(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey);
    void wgConfigsConnect(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, const QString &serverName, const QString &deviceId);

    void setIgnoreSslErrors(bool bIgnore);

signals:
    void pingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void notificationsAnswer(SERVER_API_RET_CODE retCode, QVector<types::Notification> notifications, uint userRole);

    void wgConfigsInitAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &presharedKey, const QString &allowedIps);
    void wgConfigsConnectAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &ipAddress, const QString &dnsAddress);

    void sendUserWarning(USER_WARNING_TYPE warning);
    void getRobertFiltersAnswer(SERVER_API_RET_CODE retCode, const QVector<types::RobertFilter> &robertFilters, uint userRole);
    void setRobertFilterAnswer(SERVER_API_RET_CODE retCode, uint userRole);

private:
    NetworkAccessManager *networkAccessManager_;
    types::ProxySettings proxySettings_;
    bool isProxyEnabled_;

    enum {
        NETWORK_TIMEOUT = 10000,
    };

    void handlePingTest();
    void handleNotifications();
    void handleWgConfigsInit();
    void handleWgConfigsConnect();
    void handleGetRobertFilters();
    void handleSetRobertFilter();

    void handleNetworkRequestFinished();

    IConnectStateController *connectStateController_;

    QString hostname_;
    QStringList hostIps_;

    bool bIsRequestsEnabled_;
    uint curUserRole_;
    bool bIgnoreSslErrors_;

    QHash<quint64, NetworkReply *> pingTestReplies_;

    types::ProxySettings currentProxySettings() const;
    void executeRequest(BaseRequest *request, bool isNeedCheckRequestsEnabled);
};

} // namespace server_api
