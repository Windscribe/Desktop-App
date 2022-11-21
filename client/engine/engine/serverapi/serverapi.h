#pragma once

#include <QObject>
#include <QPointer>
#include <QQueue>

#include "types/protocol.h"
#include "types/robertfilter.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "requests/baserequest.h"
#include "engine/failover/ifailover.h"

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
    // Ownership of the failover passes to the serverAPI object
    explicit ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager,
                       INetworkDetectionManager *networkDetectionManager, failover::IFailover *failover);
    virtual ~ServerAPI();

    QString getHostname() const;
    void setApiResolutionsSettings(const types::ApiResolutionSettings &apiResolutionSettings);
    void setIgnoreSslErrors(bool bIgnore);
    void resetFailover();

    BaseRequest *login(const QString &username, const QString &password, const QString &code2fa);
    BaseRequest *session(const QString &authHash);
    BaseRequest *serverLocations(const QString &language, const QString &revision, bool isPro, const QStringList &alcList);
    BaseRequest *serverCredentials(const QString &authHash, types::Protocol protocol);
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
    void onFailoverNextHostnameAnswer(failover::FailoverRetCode retCode, const QString &hostname);
    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);

private:
    NetworkAccessManager *networkAccessManager_;
    IConnectStateController *connectStateController_;
    INetworkDetectionManager *networkDetectionManager_;
    bool bIgnoreSslErrors_;
    bool isUsingFailoverFromSettings_ = false;
    bool bWasConnectedState_ = false;

    QQueue<QPointer<BaseRequest> > queueRequests_;    // a queue of requests that are waiting for the failover to complete

    // Current failover state. If there is no failover currently, then all are zero
    QPointer<BaseRequest> currentFailoverRequest_;
    ConnectStateWatcher *currentConnectStateWatcher_;
    QString currentFailoverHostname_;

    failover::IFailover *failover_;
    bool isGettingFailoverHostnameInProgress_ = false;
    bool isResetFailoverOnNextHostnameAnswer_ = false;
    bool isFailoverFailedLogAlreadyDone_ = false;   // log "failover failed: API not ready" only once to avoid spam

    void handleNetworkRequestFinished();
    void executeRequest(BaseRequest *request);

    void executeWaitingInQueueRequests();
    void finishWaitingInQueueRequests(SERVER_API_RET_CODE retCode, const QString &errString);

    void setErrorCodeAndEmitRequestFinished(BaseRequest *request, SERVER_API_RET_CODE retCode, const QString &errorStr);

    void setCurrentFailoverRequest(BaseRequest *request);
    void clearCurrentFailoverRequest();
    bool isDisconnectedState() const;

    QString hostnameForConnectedState() const;

    // Save and read the hostname from the settings where it is stored encrypted
    static constexpr quint64 SIMPLE_CRYPT_KEY = 0x2572241DF31F32EE;
    void writeHostnameToSettings(const QString &domainName);
    QString readHostnameFromSettings() const;
};

} // namespace server_api
