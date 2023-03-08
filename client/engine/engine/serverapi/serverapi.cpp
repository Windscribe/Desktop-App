#include "serverapi.h"

#include <QUrl>
#include <QUrlQuery>

#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/extraconfig.h"
#include "utils/simplecrypt.h"
#include "utils/hardcodedsettings.h"
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

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager,
                     INetworkDetectionManager *networkDetectionManager, failover::IFailoverContainer *failoverContainer) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    networkDetectionManager_(networkDetectionManager),
    bIgnoreSslErrors_(false),
    failoverState_(FailoverState::kUnknown),
    failoverContainer_(failoverContainer)
{
    connect(connectStateController_, &IConnectStateController::stateChanged, this, &ServerAPI::onConnectStateChanged);

    failoverContainer_->setParent(this);

    /*failoverFromSettingsId_ = readFailoverIdFromSettings();
    if (!failoverContainer_->failoverById(failoverFromSettingsId_))
        failoverFromSettingsId_.clear();
    else
        failoverState_ = FailoverState::kFromSettingsUnknown;*/
}

ServerAPI::~ServerAPI()
{
    requestExecutorViaFailover_.reset();
}

QString ServerAPI::getHostname() const
{
    //TODO:
    //if (!isDisconnectedState() || currentHostname_.isEmpty())
        return hostnameForConnectedState();
    //else
    //    return currentHostname_;
}

void ServerAPI::setApiResolutionsSettings(const types::ApiResolutionSettings &apiResolutionSettings)
{
    // we use it only for the disconnected mode
    apiResolutionSettings_ = apiResolutionSettings;
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

BaseRequest *ServerAPI::serverCredentials(const QString &authHash, types::Protocol protocol)
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

void ServerAPI::onNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    QPointer<BaseRequest> pointerToRequest = reply->property("pointerToRequest").value<QPointer<BaseRequest> >();

    // if the request has already been deleted before completion, skip processing
    if (!pointerToRequest) {
        return;
    }

    if (!reply->isSuccess()) {
        setErrorCodeAndEmitRequestFinished(pointerToRequest, SERVER_RETURN_NETWORK_ERROR, reply->errorString());
    }
    else {  // if reply->isSuccess()
        QByteArray serverResponse = reply->readAll();
        if (ExtraConfig::instance().getLogAPIResponse()) {
            qCDebug(LOG_SERVER_API) << pointerToRequest->name();
            qCDebugMultiline(LOG_SERVER_API) << serverResponse;
        }
        pointerToRequest->handle(serverResponse);
        emit pointerToRequest->finished();
    }
}

void ServerAPI::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location)
{
    // If we use the hostname from the settings then reset it after the first disconnect signal after starting the program
    if (failoverState_ == FailoverState::kFromSettingsReady) {
        if (state == CONNECT_STATE_CONNECTED) {
            bWasConnectedState_ = true;
        } else if (state == CONNECT_STATE_DISCONNECTED && bWasConnectedState_) {
            failoverFromSettingsId_.clear();
            failoverState_ = FailoverState::kUnknown;
        }
    }
}

void ServerAPI::onRequestExecuterViaFailoverFinished(RequestExecuterRetCode retCode)
{
    WS_ASSERT(failoverState_ == FailoverState::kUnknown || failoverState_ == FailoverState::kFromSettingsUnknown);
    QPointer<BaseRequest> request = requestExecutorViaFailover_->request();

    if (retCode == RequestExecuterRetCode::kSuccess) {
        if (failoverState_ == FailoverState::kFromSettingsUnknown) {
            failoverState_ = FailoverState::kFromSettingsReady;
            failoverFromSettingsId_.clear();
        } else {
            failoverState_ = FailoverState::kReady;
            writeFailoverIdToSettings(failoverContainer_->currentFailover()->uniqueId());
        }
        failoverData_.reset(new failover::FailoverData(requestExecutorViaFailover_->failoverData()));
        requestExecutorViaFailover_.reset();
        emit request->finished();
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kRequestDeleted) {
        WS_ASSERT(request.isNull());
        requestExecutorViaFailover_.reset();
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kFailoverFailed) {
        requestExecutorViaFailover_.reset();
        if (failoverState_ == FailoverState::kFromSettingsUnknown) {
            failoverState_ = FailoverState::kUnknown;
            failoverFromSettingsId_.clear();
            executeRequest(request);
        } else {
            WS_ASSERT(failoverState_ == FailoverState::kUnknown);
            if (failoverContainer_->gotoNext()) {
                executeRequest(request);
            } else {
                failoverState_ = FailoverState::kFailed;
                setErrorCodeAndEmitRequestFinished(request, SERVER_RETURN_FAILOVER_FAILED, "Failover API not ready");
                finishWaitingInQueueRequests(SERVER_RETURN_FAILOVER_FAILED, "Failover API not ready");
            }
        }
    } else if (retCode == RequestExecuterRetCode::kConnectStateChanged) {
        // Repeat the execution of the request via failover
        requestExecutorViaFailover_.reset();
        executeRequest(request);
    } else {
        WS_ASSERT(false);
    }
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

void ServerAPI::resetFailover()
{
    failoverContainer_->reset();
    if (failoverState_ != FailoverState::kFromSettingsUnknown) {
        failoverState_ = FailoverState::kUnknown;
        failoverFromSettingsId_.clear();
    }
}

// execute request if the failover detected or queue
void ServerAPI::executeRequest(QPointer<BaseRequest> request)
{
    // Make sure the network return code is reset if we've failed over
    request->setNetworkRetCode(SERVER_RETURN_SUCCESS);

    // check we are online
    if (!networkDetectionManager_->isOnline()) {
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: no network connection";
            if (request) {
                request->setNetworkRetCode(SERVER_RETURN_NO_NETWORK_CONNECTION);
                emit request->finished();
            }
        });
        return;
    }

    // if API resolution settings settled then use IP from the user settings
    if (!apiResolutionSettings_.getIsAutomatic() && !apiResolutionSettings_.getManualAddress().isEmpty()) {
        executeRequestImpl(request, failover::FailoverData(apiResolutionSettings_.getManualAddress()));
        executeWaitingInQueueRequests();
        return;
    }

    // if failover already in progress then move the request to queue
    if (requestExecutorViaFailover_ != nullptr) {
        // wgConfigsInit, wgConfigsConnect and pingTest should have a higher priority in the queue to avoid potential connection delays
        if (dynamic_cast<PingTestRequest *>(request.get()) != nullptr ||
            dynamic_cast<WgConfigsInitRequest *>(request.get()) != nullptr ||
            dynamic_cast<WgConfigsConnectRequest *>(request.get()) != nullptr ) {
            queueRequests_.insert(0, request);
        } else {
            queueRequests_.enqueue(request);
        }
        return;
    }

    if (!isDisconnectedState()) {
        // in the connected mode always use the primary domain
        executeRequestImpl(request, failover::FailoverData(hostnameForConnectedState()));
        executeWaitingInQueueRequests();
    } else {
        if (failoverState_ == FailoverState::kFromSettingsUnknown || failoverState_ == FailoverState::kUnknown) {
            WS_ASSERT(requestExecutorViaFailover_ == nullptr);
            int failoverInd = -1;
            QSharedPointer<failover::BaseFailover> curFailover = (failoverState_ == FailoverState::kFromSettingsUnknown) ? failoverContainer_->failoverById(failoverFromSettingsId_) : failoverContainer_->currentFailover(&failoverInd);
            WS_ASSERT(curFailover);
            qCDebug(LOG_FAILOVER) << "Trying:" << curFailover->name();
            requestExecutorViaFailover_.reset(new RequestExecuterViaFailover(this, connectStateController_, networkAccessManager_));
            connect(requestExecutorViaFailover_.get(), &RequestExecuterViaFailover::finished, this, &ServerAPI::onRequestExecuterViaFailoverFinished, Qt::QueuedConnection);
            requestExecutorViaFailover_->execute(request, curFailover, bIgnoreSslErrors_);
            // Do not emit this signal for the first failover and for from the settings failover
            if (failoverInd > 0)
                emit tryingBackupEndpoint(failoverInd, failoverContainer_->count() - 1);
        } else if (failoverState_ == FailoverState::kReady || failoverState_ == FailoverState::kFromSettingsReady) {
            WS_ASSERT(!failoverData_.isNull());
            executeRequestImpl(request, *failoverData_);
        } else if (failoverState_ == FailoverState::kFailed) {
            QTimer::singleShot(0, this, [request, this] () {
                if (!isFailoverFailedLogAlreadyDone_) {
                    qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
                    isFailoverFailedLogAlreadyDone_ = true;
                }
                if (request) {
                    request->setNetworkRetCode(SERVER_RETURN_FAILOVER_FAILED);
                    emit request->finished();
                }
            });
        }
    }
}

void ServerAPI::executeRequestImpl(QPointer<BaseRequest> request, const failover::FailoverData &failoverData)
{
    // Make sure the network return code is reset if we've failed over
    request->setNetworkRetCode(SERVER_RETURN_SUCCESS);

    NetworkRequest networkRequest(request->url(failoverData.domain()).toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_);
    if (!failoverData.echConfig().isEmpty()) {
        networkRequest.setEchConfig(failoverData.echConfig());
    }

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
    QPointer<BaseRequest> pointerToRequest(request);
    reply->setProperty("pointerToRequest",  QVariant::fromValue(pointerToRequest));
    connect(reply, &NetworkReply::finished, this, &ServerAPI::onNetworkRequestFinished);
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

void ServerAPI::setErrorCodeAndEmitRequestFinished(BaseRequest *request, SERVER_API_RET_CODE retCode, const QString &errorStr)
{
    request->setNetworkRetCode(retCode);
    if (request->isWriteToLog())
        qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed:" << errorStr;
    emit request->finished();
}

bool ServerAPI::isDisconnectedState() const
{
    // we consider these two states as disconnected from VPN
    return connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED || connectStateController_->currentState() == CONNECT_STATE_CONNECTING;
}

QString ServerAPI::hostnameForConnectedState() const
{
    return HardcodedSettings::instance().primaryServerDomain();
}

void ServerAPI::writeFailoverIdToSettings(const QString &failoverId)
{
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("flvId", simpleCrypt.encryptToString(failoverId));
}

QString ServerAPI::readFailoverIdFromSettings() const
{
    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QString str = settings.value("flvId", "").toString();
    if (!str.isEmpty())
        return simpleCrypt.decryptToString(str);
    else
        return QString();
}

} // namespace server_api
