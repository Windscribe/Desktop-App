#include "apiresourcesmanager.h"
#include "engine/getdeviceid.h"
#include "engine/openvpnversioncontroller.h"
#include "api_responses/servercredentials.h"
#include "api_responses/serverlist.h"
#include "utils/utils.h"

namespace api_resources {

using namespace wsnet;

ApiResourcesManager::ApiResourcesManager(QObject *parent, IConnectStateController *connectStateController, INetworkDetectionManager *networkDetectionManager) : QObject(parent),
    connectStateController_(connectStateController)
{
    waitForNetworkConnectivity_ = new WaitForNetworkConnectivity(this, networkDetectionManager);
    connect(waitForNetworkConnectivity_, &WaitForNetworkConnectivity::connectivityOnline, this, &ApiResourcesManager::onConnectivityOnline);
    connect(waitForNetworkConnectivity_, &WaitForNetworkConnectivity::timeoutExpired, this, &ApiResourcesManager::onConnectivityTimeoutExpired);

    fetchTimer_ = new QTimer(this);
    connect(fetchTimer_, &QTimer::timeout, this, &ApiResourcesManager::onFetchTimer);
}

ApiResourcesManager::~ApiResourcesManager()
{
    for (auto &it : requestsInProgress_)
        it->cancel();
}

void ApiResourcesManager::fetchAllWithAuthHash()
{
    waitForNetworkConnectivity_->setProperty("isLoginWithAuthHash", true);
    waitForNetworkConnectivity_->wait(kWaitTimeForNoNetwork);
}

void ApiResourcesManager::login(const QString &username, const QString &password, const QString &code2fa)
{
    waitForNetworkConnectivity_->setProperty("isLoginWithAuthHash", false);
    waitForNetworkConnectivity_->setProperty("username", username);
    waitForNetworkConnectivity_->setProperty("password", password);
    waitForNetworkConnectivity_->setProperty("code2fa", code2fa);
    waitForNetworkConnectivity_->wait(kWaitTimeForNoNetwork);
}

void ApiResourcesManager::fetchSession()
{
    lastUpdateTimeMs_.remove(RequestType::kSessionStatus);
}

void ApiResourcesManager::fetchServerCredentials()
{
    WS_ASSERT(!isFetchingServerCredentials_);
    isFetchingServerCredentials_ = true;
    isOpenVpnCredentialsReceived_ = false;
    isIkev2CredentialsReceived_ = false;
    isServerConfigsReceived_ = false;

    lastUpdateTimeMs_.remove(RequestType::kServerCredentialsOpenVPN);
    lastUpdateTimeMs_.remove(RequestType::kServerCredentialsIkev2);
    lastUpdateTimeMs_.remove(RequestType::kServerConfigs);

    fetchServerCredentialsOpenVpn(apiInfo_.getAuthHash());
    fetchServerCredentialsIkev2(apiInfo_.getAuthHash());
    fetchServerConfigs(apiInfo_.getAuthHash());
}

bool ApiResourcesManager::loadFromSettings()
{
    return apiInfo_.loadFromSettings();
}

bool ApiResourcesManager::isLoggedIn() const
{
    return fetchTimer_->isActive();
}

void ApiResourcesManager::setServerCredentials(const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig)
{
    apiInfo_.setServerCredentials(serverCredentials);
    apiInfo_.setOvpnConfig(serverConfig);
}

bool ApiResourcesManager::isCanBeLoadFromSettings()
{
    if (!apiinfo::ApiInfo::getAuthHash().isEmpty()) {
        apiinfo::ApiInfo apiInfo;
        if (apiInfo.loadFromSettings())
            return true;
    }
    return false;
}

void ApiResourcesManager::onInitialSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    requestsInProgress_.remove(RequestType::kSessionStatus);
    if (serverApiRetCode == ServerApiRetCode::kNetworkError) {
        // repeat the request
        fetchAllWithAuthHash();
    } else {
        handleLoginOrSessionAnswer(serverApiRetCode, jsonData);
    }
}

void ApiResourcesManager::onLoginAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData,
                                        const QString &username, const QString &password, const QString &code2fa)
{
    requestsInProgress_.remove(RequestType::kSessionStatus);
    if (serverApiRetCode == ServerApiRetCode::kNetworkError) {
        // repeat the request
        loginImpl(username, password, code2fa);
    } else {
        handleLoginOrSessionAnswer(serverApiRetCode, jsonData);
    }
}

void ApiResourcesManager::onServerConfigsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        QByteArray ovpnConfig = QByteArray::fromBase64(QByteArray(jsonData.c_str()));
        apiInfo_.setOvpnConfig(ovpnConfig);
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kServerConfigs] = QDateTime::currentMSecsSinceEpoch();
        isServerConfigsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerConfigs);
}

void ApiResourcesManager::onServerCredentialsOpenVpnAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::ServerCredentials sc(jsonData);
        apiInfo_.setServerCredentialsOpenVpn(sc.username(), sc.password());
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kServerCredentialsOpenVPN] = QDateTime::currentMSecsSinceEpoch();
        isOpenVpnCredentialsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerCredentialsOpenVPN);
}

void ApiResourcesManager::onServerCredentialsIkev2Answer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::ServerCredentials sc(jsonData);
        apiInfo_.setServerCredentialsIkev2(sc.username(), sc.password());
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kServerCredentialsIkev2] = QDateTime::currentMSecsSinceEpoch();
        isIkev2CredentialsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kServerCredentialsIkev2);
}

void ApiResourcesManager::onServerLocationsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::ServerList sl(jsonData);
        apiInfo_.setLocations(sl.locations());
        apiInfo_.setForceDisconnectNodes(sl.forceDisconnectNodes());
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kLocations] = QDateTime::currentMSecsSinceEpoch();
        emit locationsUpdated(sl.countryOverride());
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kLocations);
}

void ApiResourcesManager::onPortMapAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::PortMap portMap(jsonData);
        apiInfo_.setPortMap(portMap);
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kPortMap] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kPortMap);
}

void ApiResourcesManager::onStaticIpsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::StaticIps si(jsonData);
        apiInfo_.setStaticIps(si);
        saveApiInfoToSettings();
        lastUpdateTimeMs_[RequestType::kStaticIps] = QDateTime::currentMSecsSinceEpoch();
        emit staticIpsUpdated();
        checkForReadyLogin();
    }
    requestsInProgress_.remove(RequestType::kStaticIps);
}

void ApiResourcesManager::onNotificationsAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
            api_responses::Notifications n(jsonData);
        emit notificationsUpdated(n.notifications());
        lastUpdateTimeMs_[RequestType::kNotifications] = QDateTime::currentMSecsSinceEpoch();
    }
    requestsInProgress_.remove(RequestType::kNotifications);
}

void ApiResourcesManager::onSessionAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        api_responses::SessionStatus ss(jsonData);
        if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kSuccess) {
            apiInfo_.setSessionStatus(ss);
            saveApiInfoToSettings();
            updateSessionStatus();
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kSessionInvalid) {
            emit sessionDeleted();
        }
        lastUpdateTimeMs_[RequestType::kSessionStatus] = QDateTime::currentMSecsSinceEpoch();
    }
    requestsInProgress_.remove(RequestType::kSessionStatus);
}

void ApiResourcesManager::onFetchTimer()
{
    WS_ASSERT(!apiInfo_.getAuthHash().isEmpty());
    if (!apiInfo_.getAuthHash().isEmpty())
        fetchAll(apiInfo_.getAuthHash());
}

void ApiResourcesManager::onConnectivityOnline()
{
    if (waitForNetworkConnectivity_->property("isLoginWithAuthHash").toBool()) {
        fetchAllWithAuthHashImpl();
    } else {
        loginImpl(waitForNetworkConnectivity_->property("username").toString(), waitForNetworkConnectivity_->property("password").toString(),
                  waitForNetworkConnectivity_->property("code2fa").toString());
    }
}

void ApiResourcesManager::onConnectivityTimeoutExpired()
{
    emit loginFailed(LOGIN_RET_NO_CONNECTIVITY, QString());
}

void ApiResourcesManager::handleLoginOrSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess)  {
        api_responses::SessionStatus ss(jsonData);

        if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kSuccess) {
            if (!ss.getAuthHash().isEmpty()) {
                apiInfo_.setAuthHash(ss.getAuthHash());
            }
            apiInfo_.setSessionStatus(ss);
            saveApiInfoToSettings();
            lastUpdateTimeMs_[RequestType::kSessionStatus] = QDateTime::currentMSecsSinceEpoch();
            updateSessionStatus();
            checkForReadyLogin();
            fetchAll(ss.getAuthHash());
            fetchTimer_->start(1000);
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kBadUsername) {
            emit loginFailed(LOGIN_RET_BAD_USERNAME, ss.getErrorMessage());
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kMissingCode2FA) {
            emit loginFailed(LOGIN_RET_MISSING_CODE2FA, ss.getErrorMessage());
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kBadCode2FA) {
            emit loginFailed(LOGIN_RET_BAD_CODE2FA, ss.getErrorMessage());
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kAccountDisabled) {
            emit loginFailed(LOGIN_RET_ACCOUNT_DISABLED, ss.getErrorMessage());
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kSessionInvalid) {
            emit loginFailed(LOGIN_RET_SESSION_INVALID, ss.getErrorMessage());
        } else if (ss.getSessionErrorCode() == api_responses::SessionErrorCode::kRateLimited) {
            emit loginFailed(LOGIN_RET_RATE_LIMITED, ss.getErrorMessage());
        } else {
            WS_ASSERT(false);
            emit loginFailed(LOGIN_RET_NO_API_CONNECTIVITY, QString());
        }
    } else if (serverApiRetCode == ServerApiRetCode::kNoNetworkConnection) {
        emit loginFailed(LOGIN_RET_NO_CONNECTIVITY, QString());
    } else if (serverApiRetCode == ServerApiRetCode::kIncorrectJson) {
        emit loginFailed(LOGIN_RET_INCORRECT_JSON, QString());
    } else if (serverApiRetCode == ServerApiRetCode::kFailoverFailed) {
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

void ApiResourcesManager::checkForServerCredentialsFetchFinished()
{
    if (isFetchingServerCredentials_ && isOpenVpnCredentialsReceived_ && isIkev2CredentialsReceived_ && isServerConfigsReceived_) {
        isFetchingServerCredentials_ = false;
        emit serverCredentialsFetched();
    }
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
}

void ApiResourcesManager::fetchAllWithAuthHashImpl()
{
    WS_ASSERT(!apiInfo_.getAuthHash().isEmpty());
    WS_ASSERT(!requestsInProgress_.contains(RequestType::kSessionStatus));

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onInitialSessionAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kSessionStatus] = WSNet::instance()->serverAPI()->session(apiInfo_.getAuthHash().toStdString(), callback);
}

void ApiResourcesManager::loginImpl(const QString &username, const QString &password, const QString &code2fa)
{
    WS_ASSERT(!requestsInProgress_.contains(RequestType::kSessionStatus));

    auto callback = [this, username, password, code2fa](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData, username, password, code2fa] {
            onLoginAnswer(serverApiRetCode, jsonData, username, password, code2fa);
        });
    };
    requestsInProgress_[RequestType::kSessionStatus] = WSNet::instance()->serverAPI()->login(username.toStdString(),
                                                                                             password.toStdString(),
                                                                                             code2fa.toStdString(),
                                                                                             callback);
}

void ApiResourcesManager::fetchServerConfigs(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerConfigs))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onServerConfigsAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kServerConfigs] = WSNet::instance()->serverAPI()->serverConfigs(apiInfo_.getAuthHash().toStdString(),
                                                                                                     OpenVpnVersionController::instance().getOpenVpnVersion().toStdString(),
                                                                                                     callback);
}

void ApiResourcesManager::fetchServerCredentialsOpenVpn(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerCredentialsOpenVPN))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onServerCredentialsOpenVpnAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kServerCredentialsOpenVPN] = WSNet::instance()->serverAPI()->serverCredentials(apiInfo_.getAuthHash().toStdString(), true, callback);
}

void ApiResourcesManager::fetchServerCredentialsIkev2(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kServerCredentialsIkev2))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onServerCredentialsIkev2Answer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kServerCredentialsIkev2] = WSNet::instance()->serverAPI()->serverCredentials(apiInfo_.getAuthHash().toStdString(), false, callback);
}

void ApiResourcesManager::fetchLocations()
{
    if (requestsInProgress_.contains(RequestType::kLocations))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onServerLocationsAnswer(serverApiRetCode, jsonData);
        });
    };

    // convert QStringList to std::vector<std::string>
    std::vector<std::string> alcList;
    const auto &l = apiInfo_.getSessionStatus().getAlc();
    for (const auto &it : l) {
        alcList.push_back(it.toStdString());
    }

    requestsInProgress_[RequestType::kLocations] = WSNet::instance()->serverAPI()->serverLocations("en", apiInfo_.getSessionStatus().getRevisionHash().toStdString(),
                                                                                                   apiInfo_.getSessionStatus().isPremium(), alcList, callback);
}

void ApiResourcesManager::fetchPortMap(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kPortMap))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onPortMapAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kPortMap] = WSNet::instance()->serverAPI()->portMap(apiInfo_.getAuthHash().toStdString(), 6, std::vector<std::string>(), callback);
}

void ApiResourcesManager::fetchStaticIps(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kStaticIps))
        return;

    if (apiInfo_.getSessionStatus().getStaticIpsCount() > 0) {

        auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
            QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
                onStaticIpsAnswer(serverApiRetCode, jsonData);
            });
        };
        requestsInProgress_[RequestType::kStaticIps] = WSNet::instance()->serverAPI()->staticIps(apiInfo_.getAuthHash().toStdString(), Utils::getBasePlatformName().toStdString(),
                                                                                                 GetDeviceId::instance().getDeviceId().toStdString(),
                                                                                                 callback);
    } else {
        apiInfo_.setStaticIps(api_responses::StaticIps());
        lastUpdateTimeMs_[RequestType::kStaticIps] = QDateTime::currentMSecsSinceEpoch();
        checkForReadyLogin();
    }
}

void ApiResourcesManager::fetchNotifications(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kNotifications))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onNotificationsAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kNotifications] = WSNet::instance()->serverAPI()->notifications(apiInfo_.getAuthHash().toStdString(), std::string(), callback);
}

void ApiResourcesManager::fetchSession(const QString &authHash)
{
    if (requestsInProgress_.contains(RequestType::kSessionStatus))
        return;

    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        QMetaObject::invokeMethod(this, [this, serverApiRetCode, jsonData] {
            onSessionAnswer(serverApiRetCode, jsonData);
        });
    };
    requestsInProgress_[RequestType::kSessionStatus] = WSNet::instance()->serverAPI()->session(authHash.toStdString(), callback);
}

void ApiResourcesManager::updateSessionStatus()
{
    const api_responses::SessionStatus ss = apiInfo_.getSessionStatus();

    if (ss.isChangedForLogging(prevSessionForLogging_)) {
        qCDebug(LOG_BASIC) << "update session status (changed since last call)";
        qCDebugMultiline(LOG_BASIC) << ss.debugString();
        prevSessionForLogging_ = ss;
    }

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

        if (prevSessionStatus_.getStatus() == 2 && ss.getStatus() == 1) {
            fetchServerCredentialsOpenVpn(apiInfo_.getAuthHash());
            fetchServerCredentialsIkev2(apiInfo_.getAuthHash());
        }
    }

    prevSessionStatus_ = ss;
    emit sessionUpdated(ss);
}

void ApiResourcesManager::saveApiInfoToSettings()
{
    if (apiInfo_.isEverythingInit())
        apiInfo_.saveToSettings();
}


} // namespace api_resources
