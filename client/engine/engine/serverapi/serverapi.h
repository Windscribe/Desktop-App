#pragma once

#include <QObject>

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

Also this class makes a failover algorithm for all requests, which depends on the VPN connection status.
Important: while the failover domain is not detected, requests can wait in the queue.
*/

class ServerAPI : public QObject
{
    Q_OBJECT
public:
    explicit ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);
    virtual ~ServerAPI();

    // if true, then all requests works
    // if false, then only request for SERVER_API_ROLE_LOGIN_CONTROLLER and SERVER_API_ROLE_ACCESS_IPS_CONTROLLER roles works
    void setRequestsEnabled(bool bEnable);
    bool isRequestsEnabled() const;

    // set single hostname for make API requests
    void setHostname(const QString &hostname);
    QString getHostname() const;

    void setIgnoreSslErrors(bool bIgnore);

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

    BaseRequest *pingTest(uint timeout, bool bWriteLog);

    BaseRequest *notifications(const QString &authHash, bool isNeedCheckRequestsEnabled);

    BaseRequest *getRobertFilters(const QString &authHash, bool isNeedCheckRequestsEnabled);
    BaseRequest *setRobertFilter(const QString &authHash, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter);

    BaseRequest *wgConfigsInit(const QString &authHash, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey);
    BaseRequest *wgConfigsConnect(const QString &authHash, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, const QString &serverName, const QString &deviceId);

private:
    NetworkAccessManager *networkAccessManager_;
    IConnectStateController *connectStateController_;
    QString hostname_;
    bool bIsRequestsEnabled_;
    bool bIgnoreSslErrors_;

    struct {
        bool isFailoverForDisconnectedDetected_ = false;
        QString hostnameForDisconnected_;
        bool isFailoverForConnectedDetected_ = false;
        QString hostnameForConnected_;
    } failoverState_;

    void handleNetworkRequestFinished();
    void executeRequest(BaseRequest *request, bool isNeedCheckRequestsEnabled);
};

} // namespace server_api
