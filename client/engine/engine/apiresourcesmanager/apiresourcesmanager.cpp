#include "apiresourcesmanager.h"

#include "engine/getdeviceid.h"
#include "engine/serverapi/requests/loginrequest.h"
#include "engine/serverapi/requests/sessionrequest.h"
#include "engine/serverapi/requests/serverconfigsrequest.h"
#include "engine/serverapi/requests/servercredentialsrequest.h"
#include "engine/serverapi/requests/serverlistrequest.h"
#include "engine/serverapi/requests/portmaprequest.h"
#include "engine/serverapi/requests/staticipsrequest.h"
#include "engine/serverapi/requests/notificationsrequest.h"
#include "engine/serverapi/requests/checkupdaterequest.h"

namespace api_resources_manager {

ApiResourcesManager::ApiResourcesManager(QObject *parent, server_api::ServerAPI *serverAPI, IConnectStateController *connectStateController) : QObject(parent),
    serverAPI_(serverAPI), connectStateController_(connectStateController)
{
    fetchTimer_ = new QTimer(this);
    connect(fetchTimer_, &QTimer::timeout, this, &ApiResourcesManager::onFetchTimer);
}

ApiResourcesManager::~ApiResourcesManager()
{
    for (const auto &it : requestsInProgress_)
        delete it;
    requestsInProgress_.clear();
}

void ApiResourcesManager::fetchAllWithAuthHash()
{
    WS_ASSERT(!apiInfo_.getAuthHash().isEmpty());
    WS_ASSERT(!requestsInProgress_.contains(RequestType::kSessionStatus));

    requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->session(apiInfo_.getAuthHash());
    connect(requestsInProgress_[RequestType::kSessionStatus], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onInitialSessionAnswer);
}

void ApiResourcesManager::login(const QString &username, const QString &password, const QString &code2fa)
{
    WS_ASSERT(!requestsInProgress_.contains(RequestType::kSessionStatus));

    requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->login(username, password, code2fa);
    requestsInProgress_[RequestType::kSessionStatus]->setProperty("username", username);
    requestsInProgress_[RequestType::kSessionStatus]->setProperty("password", password);
    requestsInProgress_[RequestType::kSessionStatus]->setProperty("code2fa", code2fa);
    connect(requestsInProgress_[RequestType::kSessionStatus], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onInitialSessionAnswer);
}

void ApiResourcesManager::fetchSessionOnForegroundEvent()
{
    lastUpdateTimeMs_.remove(RequestType::kSessionStatus);
}

void ApiResourcesManager::checkUpdate(UPDATE_CHANNEL channel)
{
    updateChannel_ = channel;
    isUpdateChannelInit_ = true;
    fetchCheckUpdate();
}

bool ApiResourcesManager::isReadyForLogin() const
{
    return apiInfo_.isEverythingInit();
}

void ApiResourcesManager::onInitialSessionAnswer()
{
    requestsInProgress_.remove(RequestType::kSessionStatus);
    QSharedPointer<server_api::SessionRequest> request(static_cast<server_api::SessionRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_NETWORK_ERROR) {
        // repeat the request
        fetchAllWithAuthHash();
    } else {
        handleLoginOrSessionAnswer(request->networkRetCode(), request->sessionErrorCode(), request->sessionStatus(), request->authHash(), QString());
    }
}

void ApiResourcesManager::onLoginAnswer()
{
    requestsInProgress_.remove(RequestType::kSessionStatus);
    QSharedPointer<server_api::LoginRequest> request(static_cast<server_api::LoginRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_NETWORK_ERROR) {
        // repeat the request
        login(request->property("username").toString(), request->property("password").toString(), request->property("code2fa").toString());
    } else {
        handleLoginOrSessionAnswer(request->networkRetCode(), request->sessionErrorCode(), request->sessionStatus(), request->authHash(), request->errorMessage());
    }
}

void ApiResourcesManager::onServerConfigsAnswer()
{
    QSharedPointer<server_api::ServerConfigsRequest> request(static_cast<server_api::ServerConfigsRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setOvpnConfig(request->ovpnConfig());
        lastUpdateTimeMs_[RequestType::kServerConfigs] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerConfigs);
}

void ApiResourcesManager::onServerCredentialsOpenVpnAnswer()
{
    QSharedPointer<server_api::ServerCredentialsRequest> request(static_cast<server_api::ServerCredentialsRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setServerCredentialsOpenVpn(request->radiusUsername(), request->radiusPassword());
        lastUpdateTimeMs_[RequestType::kServerCredentialsOpenVPN] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerCredentialsOpenVPN);
}

void ApiResourcesManager::onServerCredentialsIkev2Answer()
{
    QSharedPointer<server_api::ServerCredentialsRequest> request(static_cast<server_api::ServerCredentialsRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setServerCredentialsIkev2(request->radiusUsername(), request->radiusPassword());
        lastUpdateTimeMs_[RequestType::kServerCredentialsIkev2] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerCredentialsIkev2);
}

void ApiResourcesManager::onServerLocationsAnswer()
{
    QSharedPointer<server_api::ServerListRequest> request(static_cast<server_api::ServerListRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setLocations(request->locations());
        lastUpdateTimeMs_[RequestType::kLocations] = QDateTime::currentMSecsSinceEpoch();
        emit locationsUpdated(request->locations());
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kLocations);
}

void ApiResourcesManager::onPortMapAnswer()
{
    QSharedPointer<server_api::PortMapRequest> request(static_cast<server_api::PortMapRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setPortMap(request->portMap());
        lastUpdateTimeMs_[RequestType::kPortMap] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kPortMap);
}

void ApiResourcesManager::onStaticIpsAnswer()
{
    QSharedPointer<server_api::StaticIpsRequest> request(static_cast<server_api::StaticIpsRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        apiInfo_.setStaticIps(request->staticIps());
        lastUpdateTimeMs_[RequestType::kStaticIps] = QDateTime::currentMSecsSinceEpoch();
        emit staticIpsUpdated(request->staticIps());
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kStaticIps);
}

void ApiResourcesManager::onNotificationsAnswer()
{
    QSharedPointer<server_api::NotificationsRequest> request(static_cast<server_api::NotificationsRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        emit notificationsUpdated(request->notifications());
        lastUpdateTimeMs_[RequestType::kNotifications] = QDateTime::currentMSecsSinceEpoch();
    }
    requestsInProgress_.remove(RequestType::kNotifications);
}

void ApiResourcesManager::onSessionAnswer()
{
    QSharedPointer<server_api::SessionRequest> request(static_cast<server_api::SessionRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        if (request->sessionErrorCode() == server_api::SessionErrorCode::kSuccess) {
            apiInfo_.setSessionStatus(request->sessionStatus());
            updateSessionStatus();
        } else if (request->sessionErrorCode() == server_api::SessionErrorCode::kSessionInvalid) {
            emit sessionDeleted();
        }
        lastUpdateTimeMs_[RequestType::kSessionStatus] = QDateTime::currentMSecsSinceEpoch();
    }
    requestsInProgress_.remove(RequestType::kSessionStatus);
}

void ApiResourcesManager::onCheckUpdateAnswer()
{
    QSharedPointer<server_api::CheckUpdateRequest> request(static_cast<server_api::CheckUpdateRequest *>(sender()), &QObject::deleteLater);
    if (request->networkRetCode() == SERVER_RETURN_SUCCESS) {
        emit checkUpdateUpdated(request->checkUpdate());
        lastUpdateTimeMs_[RequestType::kCheckUpdate] = QDateTime::currentMSecsSinceEpoch();
    }
    requestsInProgress_.remove(RequestType::kCheckUpdate);
}

void ApiResourcesManager::onFetchTimer()
{
     WS_ASSERT(!apiInfo_.getAuthHash().isEmpty());
     if (!apiInfo_.getAuthHash().isEmpty())
        fetchAll(apiInfo_.getAuthHash());
}

void ApiResourcesManager::handleLoginOrSessionAnswer(SERVER_API_RET_CODE retCode, server_api::SessionErrorCode sessionErrorCode, const types::SessionStatus &sessionStatus, const QString &authHash, const QString &errorMessage)
{
    if (retCode == SERVER_RETURN_SUCCESS)  {
        if (sessionErrorCode == server_api::SessionErrorCode::kSuccess) {
            apiInfo_.setAuthHash(authHash);
            apiInfo_.setSessionStatus(sessionStatus);
            lastUpdateTimeMs_[RequestType::kSessionStatus] = QDateTime::currentMSecsSinceEpoch();
            updateSessionStatus();
            checkForReadyLogin();
            fetchAll(authHash);
            fetchTimer_->start(1000);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kBadUsername) {
            emit loginFailed(LOGIN_RET_BAD_USERNAME, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kMissingCode2FA) {
            emit loginFailed(LOGIN_RET_MISSING_CODE2FA, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kBadCode2FA) {
            emit loginFailed(LOGIN_RET_BAD_CODE2FA, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kAccountDisabled) {
            emit loginFailed(LOGIN_RET_ACCOUNT_DISABLED, errorMessage);
        } else if (sessionErrorCode == server_api::SessionErrorCode::kSessionInvalid) {
            emit loginFailed(LOGIN_RET_SESSION_INVALID, errorMessage);
        } else {
            WS_ASSERT(false);
            emit loginFailed(LOGIN_RET_NO_API_CONNECTIVITY, QString());
        }
    } else if (retCode == SERVER_RETURN_SSL_ERROR) {
        emit loginFailed(LOGIN_RET_SSL_ERROR, QString());
    } else if (retCode == SERVER_RETURN_NO_NETWORK_CONNECTION) {
        emit loginFailed(LOGIN_RET_NO_CONNECTIVITY, QString());
    } else if (retCode == SERVER_RETURN_INCORRECT_JSON) {
        emit loginFailed(LOGIN_RET_INCORRECT_JSON, QString());
    } else if (retCode == SERVER_RETURN_FAILOVER_FAILED) {
        emit loginFailed(LOGIN_RET_NO_API_CONNECTIVITY, QString());
    } else {
        WS_ASSERT(false);
    }
}

void ApiResourcesManager::checkForReadyLogin()
{
    if (apiInfo_.isEverythingInit())
        emit readyForLogin();
}

void ApiResourcesManager::fetchAll(const QString &authHash)
{
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();

    if (connectStateController_->currentState() == CONNECT_STATE_CONNECTED) {
        // fetch session every 1 min in the connected state
        if (!lastUpdateTimeMs_.contains(RequestType::kSessionStatus) || (curTime - lastUpdateTimeMs_[RequestType::kSessionStatus]) > kMinute)
            fetchSession(authHash);
    } else {
        // fetch session every 1 hour in the disconnected state
        if (!lastUpdateTimeMs_.contains(RequestType::kSessionStatus) || (curTime - lastUpdateTimeMs_[RequestType::kSessionStatus]) > kHour)
            fetchSession(authHash);
    }

    // fetch locations every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kLocations) || (curTime - lastUpdateTimeMs_[RequestType::kLocations]) > k24Hours)
        fetchLocations();

    // fetch static ips every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kStaticIps) || (curTime - lastUpdateTimeMs_[RequestType::kStaticIps]) > k24Hours)
        fetchStaticIps(authHash);

    // fetch server configs every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kServerConfigs) || (curTime - lastUpdateTimeMs_[RequestType::kServerConfigs]) > k24Hours)
        fetchServerConfigs(authHash);

    // fetch server credentials every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kServerCredentialsOpenVPN) || (curTime - lastUpdateTimeMs_[RequestType::kServerCredentialsOpenVPN]) > k24Hours)
        fetchServerCredentialsOpenVpn(authHash);
    if (!lastUpdateTimeMs_.contains(RequestType::kServerCredentialsIkev2) || (curTime - lastUpdateTimeMs_[RequestType::kServerCredentialsIkev2]) > k24Hours)
        fetchServerCredentialsIkev2(authHash);

    // fetch portmap every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kPortMap) || (curTime - lastUpdateTimeMs_[RequestType::kPortMap]) > k24Hours)
        fetchPortMap(authHash);

    // fetch notifications every 1 hour
    if (!lastUpdateTimeMs_.contains(RequestType::kNotifications) || (curTime - lastUpdateTimeMs_[RequestType::kNotifications]) > kHour)
        fetchNotifications(authHash);

    // fetch check update every 24 hours
    if (!lastUpdateTimeMs_.contains(RequestType::kCheckUpdate) || (curTime - lastUpdateTimeMs_[RequestType::kCheckUpdate]) > k24Hours)
        fetchCheckUpdate();
}

void ApiResourcesManager::fetchServerConfigs(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerConfigs))
        return;
    requestsInProgress_[RequestType::kServerConfigs] = serverAPI_->serverConfigs(authHash);
    connect(requestsInProgress_[RequestType::kServerConfigs], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onServerConfigsAnswer);
}

void ApiResourcesManager::fetchServerCredentialsOpenVpn(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerCredentialsOpenVPN))
        return;
    requestsInProgress_[RequestType::kServerCredentialsOpenVPN] = serverAPI_->serverCredentials(authHash, PROTOCOL::OPENVPN_UDP);
    connect(requestsInProgress_[RequestType::kServerCredentialsOpenVPN], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onServerCredentialsOpenVpnAnswer);
}

void ApiResourcesManager::fetchServerCredentialsIkev2(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerCredentialsIkev2))
        return;
    requestsInProgress_[RequestType::kServerCredentialsIkev2] = serverAPI_->serverCredentials(authHash, PROTOCOL::IKEV2);
    connect(requestsInProgress_[RequestType::kServerCredentialsIkev2], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onServerCredentialsIkev2Answer);
}

void ApiResourcesManager::fetchLocations()
{
    if (requestsInProgress_.contains(RequestType::kLocations))
        return;
    requestsInProgress_[RequestType::kLocations] = serverAPI_->serverLocations("en", apiInfo_.getSessionStatus().getRevisionHash(), apiInfo_.getSessionStatus().isPremium(), apiInfo_.getSessionStatus().getAlc());
    connect(requestsInProgress_[RequestType::kLocations], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onServerLocationsAnswer);
}

void ApiResourcesManager::fetchPortMap(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kPortMap))
        return;
    requestsInProgress_[RequestType::kPortMap] = serverAPI_->portMap(authHash);
    connect(requestsInProgress_[RequestType::kPortMap], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onPortMapAnswer);
}

void ApiResourcesManager::fetchStaticIps(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kStaticIps))
        return;

    if (apiInfo_.getSessionStatus().getStaticIpsCount() > 0) {
        requestsInProgress_[RequestType::kStaticIps] = serverAPI_->staticIps(authHash, GetDeviceId::instance().getDeviceId());
        connect(requestsInProgress_[RequestType::kStaticIps], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onStaticIpsAnswer);
    } else {
        apiInfo_.setStaticIps(apiinfo::StaticIps());
        lastUpdateTimeMs_[RequestType::kStaticIps] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
}

void ApiResourcesManager::fetchNotifications(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kNotifications))
        return;
    requestsInProgress_[RequestType::kNotifications] = serverAPI_->notifications(authHash);
    connect(requestsInProgress_[RequestType::kNotifications], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onNotificationsAnswer);
}

void ApiResourcesManager::fetchSession(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kSessionStatus))
        return;
    requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->session(authHash);
    connect(requestsInProgress_[RequestType::kSessionStatus], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onSessionAnswer);
}

void ApiResourcesManager::fetchCheckUpdate()
{
    if (requestsInProgress_.contains(RequestType::kCheckUpdate) || !isUpdateChannelInit_)
        return;
    requestsInProgress_[RequestType::kCheckUpdate] = serverAPI_->checkUpdate(updateChannel_);
    connect(requestsInProgress_[RequestType::kCheckUpdate], &server_api::BaseRequest::finished, this, &ApiResourcesManager::onCheckUpdateAnswer);
}

void ApiResourcesManager::updateSessionStatus()
{
    const types::SessionStatus ss = apiInfo_.getSessionStatus();

    if (ss.isChangedForLogging(prevSessionForLogging_)) {
        qCDebug(LOG_BASIC) << "update session status (changed since last call)";
        qCDebugMultiline(LOG_BASIC) << ss.debugString();
        prevSessionForLogging_ = ss;
    }

    emit sessionUpdated(ss);

    if (prevSessionStatus_.isInitialized()) {

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.isPremium() != ss.isPremium() ||
            prevSessionStatus_.getBillingPlanId() != ss.getBillingPlanId() ||
            prevSessionStatus_.getAlc() != ss.getAlc() || (prevSessionStatus_.getStatus() != 1 && ss.getStatus() == 1)) {

            fetchLocations();
        }

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.getStaticIpsCount() != ss.getStaticIpsCount() ||
            ss.isContainsStaticDeviceId(GetDeviceId::instance().getDeviceId()) ||
            prevSessionStatus_.isPremium() != ss.isPremium() ||
            prevSessionStatus_.getBillingPlanId() != ss.getBillingPlanId()) {

            fetchStaticIps(apiInfo_.getAuthHash());
        }

        if (prevSessionStatus_.isPremium() != ss.isPremium() || prevSessionStatus_.getBillingPlanId() != ss.getBillingPlanId())  {
            fetchServerCredentialsOpenVpn(apiInfo_.getAuthHash());
            fetchServerCredentialsIkev2(apiInfo_.getAuthHash());
            fetchNotifications(apiInfo_.getAuthHash());
        }
    }

    prevSessionStatus_ = ss;
}


} // namespace api_resources_manager
