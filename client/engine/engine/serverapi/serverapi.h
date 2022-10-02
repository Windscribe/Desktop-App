#pragma once

#include <QObject>
#include <QPointer>

#include "types/robertfilter.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"
#include "requests/baserequest.h"
#include "failoverwithstate.h"

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
    //FIXME: add dnsResolutionSettings_.getIsAutomatic()
    explicit ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);

    // if true, then all requests works
    // if false, then only request for SERVER_API_ROLE_LOGIN_CONTROLLER and SERVER_API_ROLE_ACCESS_IPS_CONTROLLER roles works
    void setRequestsEnabled(bool bEnable);
    bool isRequestsEnabled() const;

    // set single hostname for make API requests
    void setHostname(const QString &hostname);
    QString getHostname() const;

    void setIgnoreSslErrors(bool bIgnore);

    BaseRequest *login(const QString &username, const QString &password, const QString &code2fa);
    BaseRequest *session(const QString &authHash);
    BaseRequest *serverLocations(const QString &language, const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList);
    BaseRequest *serverCredentials(const QString &authHash, PROTOCOL protocol);
    BaseRequest *deleteSession(const QString &authHash);
    BaseRequest *serverConfigs(const QString &authHash);
    BaseRequest *portMap(const QString &authHash);
    BaseRequest *recordInstall();
    BaseRequest *confirmEmail(const QString &authHash);
    BaseRequest *webSession(const QString authHash, WEB_SESSION_PURPOSE purpose);

    BaseRequest *myIP(int timeout);

    BaseRequest *checkUpdate(UPDATE_CHANNEL updateChannel);
    BaseRequest *debugLog(const QString &username, const QString &strLog);
    BaseRequest *speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating);

    BaseRequest *staticIps(const QString &authHash, const QString &deviceId);

    BaseRequest *pingTest(uint timeout, bool bWriteLog);

    BaseRequest *notifications(const QString &authHash);

    BaseRequest *getRobertFilters(const QString &authHash);
    BaseRequest *setRobertFilter(const QString &authHash, const types::RobertFilter &filter);

    BaseRequest *wgConfigsInit(const QString &authHash, const QString &clientPublicKey, bool deleteOldestKey);
    BaseRequest *wgConfigsConnect(const QString &authHash, const QString &clientPublicKey, const QString &serverName, const QString &deviceId);
    BaseRequest *syncRobert(const QString &authHash);

private slots:
    void onFailoverNextHostnameAnswer(server_api::FailoverRetCode retCode, const QString &hostname);

private:
    NetworkAccessManager *networkAccessManager_;
    IConnectStateController *connectStateController_;
    QString hostname_;
    bool bIsRequestsEnabled_;
    bool bIgnoreSslErrors_;

    QQueue<QPointer<BaseRequest> > queueRequests_;    // a queue of requests that are waiting for the failover to complete

    // Current failover state. If there is no failover currently, then all are zero
    QPointer<BaseRequest> currentFailoverRequest_;
    FailoverWithState *failoverInProgress_;
    ConnectStateWatcher *currentConnectStateWatcher_;

    // Failovers for the connected/disconnected and their states
    FailoverWithState *failoverDisconnectedMode_;
    FailoverWithState *failoverConnectedMode_;

    void handleNetworkRequestFinished();
    void executeRequest(BaseRequest *request, bool bSkipFailoverConditions = false);
    void executeWaitingInQueueRequests();
    // return Failover depending of the connected/disconnected VPN state
    FailoverWithState *currentFailover();

    void setErrorCodeAndEmitRequestFinished(BaseRequest *request, SERVER_API_RET_CODE retCode, const QString &errorStr);
};

} // namespace server_api
