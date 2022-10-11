#include "serverapi.h"

#include <QUrl>
#include <QUrlQuery>

#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"

#include "requests/loginrequest.h"
#include "requests/sessionrequest.h"
#include "requests/serverlistrequest.h"
#include "requests/servercredentialsrequest.h"
#include "requests/deletesessionrequest.h"
#include "requests/serverconfigsrequest.h"
#include "requests/portmaprequest.h"
#include "requests/recordinstallrequest.h"
#include "requests/confirmemailrequest.h"
#include "requests/websessionrequest.h"
#include "requests/myiprequest.h"
#include "requests/checkupdaterequest.h"
#include "requests/debuglogrequest.h"
#include "requests/speedratingrequest.h"
#include "requests/staticipsrequest.h"
#include "requests/pingtestrequest.h"
#include "requests/notificationsrequest.h"
#include "requests/getrobertfiltersrequest.h"
#include "requests/setrobertfiltersrequest.h"
#include "requests/wgconfigsinitrequest.h"
#include "requests/wgconfigsconnectrequest.h"
#include "requests/syncrobertrequest.h"

#ifdef Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace server_api {

enum class FailoverState { kUnknown, kReady, kFailed };

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager,
                     INetworkDetectionManager *networkDetectionManager, failover::IFailover *failoverDisconnectedMode, failover::IFailover *failoverConnectedMode) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    networkDetectionManager_(networkDetectionManager),
    bIgnoreSslErrors_(false),
    currentFailoverRequest_(nullptr),
    failoverInProgress_(nullptr),
    currentConnectStateWatcher_(nullptr),
    failoverDisconnectedMode_(failoverDisconnectedMode),
    failoverConnectedMode_(failoverConnectedMode)
{
    failoverConnectedMode_->setParent(this);
    failoverConnectedMode_->setProperty("state", QVariant::fromValue(FailoverState::kUnknown));
    connect(failoverConnectedMode_, &failover::IFailover::nextHostnameAnswer, this, &ServerAPI::onFailoverNextHostnameAnswer);

    failoverDisconnectedMode_->setParent(this);
    failoverDisconnectedMode_->setProperty("state", QVariant::fromValue(FailoverState::kUnknown));
    connect(failoverDisconnectedMode_, &failover::IFailover::nextHostnameAnswer, this, &ServerAPI::onFailoverNextHostnameAnswer);

    connect(connectStateController, &IConnectStateController::stateChanged, this, &ServerAPI::onConnectStateChanged);
}

ServerAPI::~ServerAPI()
{
    if (currentFailoverRequest_)
        clearCurrentFailoverRequest();
}

QString ServerAPI::getHostname() const
{
    return currentFailover()->currentHostname();
}

void ServerAPI::setApiResolutionsSettings(const types::ApiResolutionSettings &apiResolutionSettings)
{
    // we use it only for the disconnected mode
    failoverDisconnectedMode_->setApiResolutionSettings(apiResolutionSettings);
    qCDebug(LOG_SERVER_API) << "ServerAPI::setApiResolutionsSettings" << apiResolutionSettings;
}

BaseRequest *ServerAPI::login(const QString &username, const QString &password, const QString &code2fa)
{
    LoginRequest *request = new LoginRequest(this, username, password, code2fa);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::session(const QString &authHash)
{
    SessionRequest *request = new SessionRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverLocations(const QString &language, const QString &revision, bool isPro, const QStringList &alcList)
{
    ServerListRequest *request = new ServerListRequest(this, language, revision, isPro, alcList, connectStateController_);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverCredentials(const QString &authHash, PROTOCOL protocol)
{
    ServerCredentialsRequest *request = new ServerCredentialsRequest(this, authHash, protocol);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::deleteSession(const QString &authHash)
{
    DeleteSessionRequest *request = new DeleteSessionRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverConfigs(const QString &authHash)
{
    ServerConfigsRequest *request = new ServerConfigsRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::portMap(const QString &authHash)
{
    PortMapRequest *request = new PortMapRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::recordInstall()
{
    RecordInstallRequest *request = new RecordInstallRequest(this);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::confirmEmail(const QString &authHash)
{
    ConfirmEmailRequest *request = new ConfirmEmailRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::webSession(const QString authHash, WEB_SESSION_PURPOSE purpose)
{
    WebSessionRequest *request = new WebSessionRequest(this, authHash, purpose);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::myIP(int timeout)
{
    MyIpRequest *request = new MyIpRequest(this, timeout);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::checkUpdate(UPDATE_CHANNEL updateChannel)
{
    CheckUpdateRequest *request = new CheckUpdateRequest(this, updateChannel);

    // This check will only be useful in the case that we expand our supported linux OSes and the platform flag is not added for that OS
    if (Utils::getPlatformName().isEmpty()) {
        qCDebug(LOG_SERVER_API) << "Check update failed: platform name is empty";
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
            request->setNetworkRetCode(SERVER_RETURN_SUCCESS);
            emit request->finished();
        });
        return request;
    }

    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::debugLog(const QString &username, const QString &strLog)
{
    DebugLogRequest *request = new DebugLogRequest(this, username, strLog);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating)
{
    SpeedRatingRequest *request = new SpeedRatingRequest(this, authHash, speedRatingHostname, ip, rating);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::staticIps(const QString &authHash, const QString &deviceId)
{
    StaticIpsRequest *request = new StaticIpsRequest(this, authHash, deviceId);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::pingTest(uint timeout, bool bWriteLog)
{
    if (bWriteLog)
        qCDebug(LOG_SERVER_API) << "Do ping test with timeout: " << timeout;

    PingTestRequest *request = new PingTestRequest(this, timeout);
    if (!bWriteLog) request->setNotWriteToLog();
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::notifications(const QString &authHash)
{
    NotificationsRequest *request = new NotificationsRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::wgConfigsInit(const QString &authHash, const QString &clientPublicKey, bool deleteOldestKey)
{
    WgConfigsInitRequest *request = new WgConfigsInitRequest(this, authHash, clientPublicKey, deleteOldestKey);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::wgConfigsConnect(const QString &authHash,
                                 const QString &clientPublicKey, const QString &serverName, const QString &deviceId)
{
    WgConfigsConnectRequest *request = new WgConfigsConnectRequest(this, authHash, clientPublicKey, serverName, deviceId);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::getRobertFilters(const QString &authHash)
{
    GetRobertFiltersRequest *request = new GetRobertFiltersRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::setRobertFilter(const QString &authHash, const types::RobertFilter &filter)
{
    SetRobertFiltersRequest *request = new SetRobertFiltersRequest(this, authHash, filter);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::syncRobert(const QString &authHash)
{
    SyncRobertRequest *request = new SyncRobertRequest(this, authHash);
    executeRequest(request);
    return request;
}

void ServerAPI::onFailoverNextHostnameAnswer(failover::FailoverRetCode retCode, const QString &hostname)
{
    WS_ASSERT(currentFailoverRequest_ != nullptr)
    if (retCode == failover::FailoverRetCode::kSuccess) {
        // try to repeat the request
        failoverInProgress_ = currentFailover();
        executeRequest(currentFailoverRequest_, true);
    } else if (retCode == failover::FailoverRetCode::kSslError) {
        setErrorCodeAndEmitRequestFinished(currentFailoverRequest_, SERVER_RETURN_SSL_ERROR, "Failover return Ssl error");
        finishWaitingInQueueRequests(SERVER_RETURN_SSL_ERROR, "Failover return Ssl error");
    } else if (retCode == failover::FailoverRetCode::kFailed) {
        currentFailover()->setProperty("state", QVariant::fromValue(FailoverState::kFailed));
        setErrorCodeAndEmitRequestFinished(currentFailoverRequest_, SERVER_RETURN_FAILOVER_FAILED, "Failover API not ready");
        finishWaitingInQueueRequests(SERVER_RETURN_FAILOVER_FAILED, "Failover API not ready");
    }
}

void ServerAPI::onConnectStateChanged()
{
    if (connectStateController_->currentState() == CONNECT_STATE_CONNECTED) {
        QSignalBlocker blocker(failoverConnectedMode_);
        failoverConnectedMode_->reset();
        failoverConnectedMode_->setProperty("state", QVariant::fromValue(FailoverState::kUnknown));
    }
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

void ServerAPI::handleNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    QPointer<BaseRequest> pointerToRequest = reply->property("pointerToRequest").value<QPointer<BaseRequest> >();

    // if the request has already been deleted before completion, skip processing
    if (!pointerToRequest) {
        return;
    }

    if (!reply->isSuccess()) {
        if (reply->error() == NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_) {
            setErrorCodeAndEmitRequestFinished(pointerToRequest, SERVER_RETURN_SSL_ERROR, reply->errorString());
            if (currentFailoverRequest_ == pointerToRequest) {
                clearCurrentFailoverRequest();
                executeWaitingInQueueRequests();
            }
        } else {
            if (currentFailoverRequest_ == pointerToRequest) {
                if (!currentConnectStateWatcher_->isVpnConnectStateChanged()) {
                    WS_ASSERT(failoverInProgress_ == currentFailover());
                    // get next the failover hostname
                    currentFailover()->getNextHostname(bIgnoreSslErrors_);
                } else {
                    setErrorCodeAndEmitRequestFinished(pointerToRequest, SERVER_RETURN_NETWORK_ERROR, reply->errorString());
                    clearCurrentFailoverRequest();
                    executeWaitingInQueueRequests();
                }
            } else {
                setErrorCodeAndEmitRequestFinished(pointerToRequest, SERVER_RETURN_NETWORK_ERROR, reply->errorString());
            }
        }
    }
    else {  // if reply->isSuccess()
        pointerToRequest->handle(reply->readAll());
        emit pointerToRequest->finished();

        // if for the current request we performed the failover algorithm, then set the state of failover to the kReady
        // and execute pending requests
        if (currentFailoverRequest_ == pointerToRequest) {
            if (!currentConnectStateWatcher_->isVpnConnectStateChanged()) {
                WS_ASSERT(failoverInProgress_ == currentFailover());
                currentFailover()->setProperty("state", QVariant::fromValue(FailoverState::kReady));
            }
            clearCurrentFailoverRequest();
            executeWaitingInQueueRequests();
        } else {
            WS_ASSERT(currentFailoverRequest_ == nullptr);
        }
    }
}

// execute request if the failover detected or queue
void ServerAPI::executeRequest(BaseRequest *request, bool bSkipFailoverConditions /*= false*/)
{
    if (!networkDetectionManager_->isOnline()) {
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: no network connection";
            request->setNetworkRetCode(SERVER_RETURN_NO_NETWORK_CONNECTION);
            emit request->finished();
        });
        return;
    }

    if (!bSkipFailoverConditions) {
        if (currentFailover()->property("state").value<FailoverState>() == FailoverState::kUnknown) {
            // if failover already in progress then move the request to queue
            if (currentFailoverRequest_ != nullptr) {
                queueRequests_.enqueue(request);
                return;
            } else {
                // start failover algorithm for the request
                setCurrentFailoverRequest(request, currentFailover());
            }
        }
        else if (currentFailover()->property("state").value<FailoverState>() == FailoverState::kFailed) {
            QTimer::singleShot(0, this, [request] () {
                qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
                request->setNetworkRetCode(SERVER_RETURN_FAILOVER_FAILED);
                emit request->finished();
            });
            return;
        }
    }

    //TODO: getCurrentDnsServers() move to NetworkAccessManager
    NetworkRequest networkRequest(request->url(currentFailover()->currentHostname()).toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_);
    NetworkReply *reply;
    switch (request->requestType()) {
        case RequestType::kGet:
            reply = networkAccessManager_->get(networkRequest);
            break;
        case RequestType::kPost:
            reply = networkAccessManager_->post(networkRequest, request->postData());
            break;
        case RequestType::kDelete:
            reply = networkAccessManager_->deleteResource(networkRequest);
            break;
        case RequestType::kPut:
            reply = networkAccessManager_->put(networkRequest, request->postData());
            break;
        default:
            WS_ASSERT(false);
    }

    connect(request, &BaseRequest::destroyed, reply, &NetworkReply::deleteLater);

    QPointer<BaseRequest> pointerToRequest(request);
    reply->setProperty("pointerToRequest",  QVariant::fromValue(pointerToRequest));
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleNetworkRequestFinished);
}

void ServerAPI::executeWaitingInQueueRequests()
{
    QQueue<QPointer<BaseRequest> > queueRequests = queueRequests_;
    queueRequests_.clear();
    while (!queueRequests.isEmpty()) {
        QPointer<BaseRequest> request(queueRequests.dequeue());
        if (request)
            executeRequest(request);
    }
}

void ServerAPI::finishWaitingInQueueRequests(SERVER_API_RET_CODE retCode, const QString &errString)
{
    while (!queueRequests_.isEmpty()) {
        QPointer<BaseRequest> request(queueRequests_.dequeue());
        if (request)
            setErrorCodeAndEmitRequestFinished(request, retCode, errString);
    }
    queueRequests_.clear();
}

failover::IFailover *ServerAPI::currentFailover() const
{
    if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED)
        return failoverDisconnectedMode_;
    else
        return failoverConnectedMode_;
}

void ServerAPI::setErrorCodeAndEmitRequestFinished(BaseRequest *request, SERVER_API_RET_CODE retCode, const QString &errorStr)
{
    request->setNetworkRetCode(SERVER_RETURN_NETWORK_ERROR);
    if (request->isWriteToLog())
        qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed:" << errorStr;
    emit request->finished();
}

void ServerAPI::setCurrentFailoverRequest(BaseRequest *request, failover::IFailover *failover)
{
    WS_ASSERT(currentFailoverRequest_ == nullptr);
    WS_ASSERT(failoverInProgress_ == nullptr);
    WS_ASSERT(currentConnectStateWatcher_ == nullptr);
    currentFailoverRequest_ = request;
    failoverInProgress_ = failover;
    currentConnectStateWatcher_ = new ConnectStateWatcher(this, connectStateController_);
    // if the request is deleted before completion, then we show start processing the requests waiting in the queue
    connect(request, &QObject::destroyed, [this]() {
        currentFailoverRequest_ = nullptr;
        failoverInProgress_ = nullptr;
        SAFE_DELETE(currentConnectStateWatcher_);
        executeWaitingInQueueRequests();
    });
}

void ServerAPI::clearCurrentFailoverRequest()
{
    WS_ASSERT(currentFailoverRequest_ != nullptr);
    WS_ASSERT(failoverInProgress_ != nullptr);
    WS_ASSERT(currentConnectStateWatcher_ != nullptr);

    // This is necessary here, because we connected the signal QObject::destroyed below
    // which can access the server API fields when they are already deleted
    currentFailoverRequest_->disconnect();
    currentFailoverRequest_ = nullptr;
    failoverInProgress_ = nullptr;
    SAFE_DELETE(currentConnectStateWatcher_);
}

} // namespace server_api
