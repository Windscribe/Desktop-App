#include "apiresourcesmanager.h"
#include <spdlog/spdlog.h>
#include "utils/cancelablecallback.h"
#include "utils/utils.h"
#include "settings.h"

namespace wsnet {

using namespace std::chrono;

ApiResourcesManager::ApiResourcesManager(boost::asio::io_context &io_context, WSNetServerAPI *serverAPI, PersistentSettings &persistentSettings, ConnectState &connectState) :
    io_context_(io_context),
    loginTimer_(io_context, boost::asio::chrono::seconds(1)),
    fetchTimer_(io_context, boost::asio::chrono::seconds(1)),
    serverAPI_(serverAPI),
    persistentSettings_(persistentSettings),
    connectState_(connectState)
{
    sessionStatus_.reset(SessionStatus::createFromJson(persistentSettings_.sessionStatus()));
}

ApiResourcesManager::~ApiResourcesManager()
{
    loginTimer_.cancel();
    fetchTimer_.cancel();

    for (const auto &it : requestsInProgress_) {
        it.second->cancel();
    }
}

std::shared_ptr<WSNetCancelableCallback> ApiResourcesManager::setCallback(WSNetApiResourcesManagerCallback callback)
{
    std::lock_guard locker(mutex_);
    if (callback == nullptr) {
        callback_.reset();
        return nullptr;
    } else {
        callback_ = std::make_shared<CancelableCallback<WSNetApiResourcesManagerCallback>>(callback);
        return callback_;
    }
}

void ApiResourcesManager::setAuthHash(const std::string &authHash)
{
    std::lock_guard locker(mutex_);
    persistentSettings_.setAuthHash(authHash);
}

bool ApiResourcesManager::isExist() const
{
    std::lock_guard locker(mutex_);
    return !persistentSettings_.authHash().empty() &&
            !persistentSettings_.sessionStatus().empty() &&
            !persistentSettings_.locations().empty() &&
            !persistentSettings_.serverCredentialsOvpn().empty() &&
            !persistentSettings_.serverCredentialsIkev2().empty() &&
            !persistentSettings_.serverConfigs().empty() &&
            !persistentSettings_.portMap().empty() &&
            !persistentSettings_.staticIps().empty() &&
            !persistentSettings_.notifications().empty();
}

bool ApiResourcesManager::loginWithAuthHash()
{
    std::lock_guard locker(mutex_);

    if (requestsInProgress_.find(RequestType::kSessionStatus) != requestsInProgress_.end()) {
        spdlog::error("Incorrect use of API, the function ApiResourcesManager::loginWithAuthHash is called twice");
        assert(false);
    }

    if (persistentSettings_.authHash().empty())
        return false;

    if (connectState_.isOnline()) {
        using namespace std::placeholders;
        requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->session(persistentSettings_.authHash(), appleId_, gpDeviceId_, std::bind(&ApiResourcesManager::onInitialSessionAnswer, this, _1, _2));
    } else {
        // If we're not online, do it again in a second
        if (!startLoginTime_.has_value()) {
            startLoginTime_ = std::chrono::steady_clock::now();
        } else {
            if (utils::since(*startLoginTime_).count() > kWaitTimeForNoNetwork) {
                boost::asio::post(io_context_, [this] {
                    std::lock_guard locker(mutex_);
                    callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kNoConnectivity, std::string());
                });
                return true;
            }
        }

        loginTimer_.async_wait([this] (boost::system::error_code const& err)  {
            if (!err) {
                loginWithAuthHash();
            }
        });
    }
    return true;
}

void ApiResourcesManager::login(const std::string &username, const std::string &password, const std::string &code2fa)
{
    std::lock_guard locker(mutex_);

    if (requestsInProgress_.find(RequestType::kSessionStatus) != requestsInProgress_.end()) {
        spdlog::error("Incorrect use of API, the function ApiResourcesManager::login is called twice");
        assert(false);
    }

    if (connectState_.isOnline()) {
        using namespace std::placeholders;
        requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->login(username, password, code2fa, std::bind(&ApiResourcesManager::onLoginAnswer, this, _1, _2, username, password, code2fa));
    } else {
        // If we're not online, do it again in a second
        if (!startLoginTime_.has_value()) {
            startLoginTime_ = std::chrono::steady_clock::now();
        } else {
            if (utils::since(*startLoginTime_).count() > kWaitTimeForNoNetwork) {
                boost::asio::post(io_context_, [this] {
                    std::lock_guard locker(mutex_);
                    callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kNoConnectivity, std::string());
                });
                return;
            }
        }

        loginTimer_.async_wait([this, username, password, code2fa] (boost::system::error_code const& err)  {
            if (!err) {
                login(username, password, code2fa);
            }
        });
    }
}

void ApiResourcesManager::logout()
{
    std::lock_guard locker(mutex_);
    loginTimer_.cancel();
    fetchTimer_.cancel();

    using namespace std::placeholders;
    serverAPI_->deleteSession(persistentSettings_.authHash(), std::bind(&ApiResourcesManager::onDeleteSessionAnswer, this, _1, _2));

    clearValues();
}

void ApiResourcesManager::fetchSession()
{
    std::lock_guard locker(mutex_);
    lastUpdateTimeMs_.erase(RequestType::kSessionStatus);
}

void ApiResourcesManager::fetchServerCredentials()
{
    std::lock_guard locker(mutex_);
    assert(!isFetchingServerCredentials_);
    isFetchingServerCredentials_ = true;
    isOpenVpnCredentialsReceived_ = false;
    isIkev2CredentialsReceived_ = false;
    isServerConfigsReceived_ = false;

    lastUpdateTimeMs_.erase(RequestType::kServerCredentialsOpenVPN);
    lastUpdateTimeMs_.erase(RequestType::kServerCredentialsIkev2);
    lastUpdateTimeMs_.erase(RequestType::kServerConfigs);

    auto authHash = persistentSettings_.authHash();
    fetchServerCredentialsOpenVpn(authHash);
    fetchServerCredentialsIkev2(authHash);
    fetchServerConfigs(authHash);
}

std::string ApiResourcesManager::authHash()
{
    return persistentSettings_.authHash();
}

void ApiResourcesManager::removeFromPersistentSettings()
{
    std::lock_guard locker(mutex_);
    clearValues();
}

void ApiResourcesManager::checkUpdate(UpdateChannel channel, const std::string &appVersion, const std::string &appBuild, const std::string &osVersion, const std::string &osBuild)
{
    std::lock_guard locker(mutex_);
    checkUpdateData_.channel = channel;
    checkUpdateData_.appVersion = appVersion;
    checkUpdateData_.appBuild = appBuild;
    checkUpdateData_.osVersion = osVersion;
    checkUpdateData_.osBuild = osBuild;
    lastUpdateTimeMs_.erase(RequestType::kCheckUpdate);
    isCheckUpdateDataSet_ = true;
}

void ApiResourcesManager::setNotificationPcpid(const std::string &pcpid)
{
    std::lock_guard locker(mutex_);
    pcpidNotifications_ = pcpid;
}

void ApiResourcesManager::setMobileDeviceId(const std::string &appleId, const std::string &gpDeviceId)
{
    std::lock_guard locker(mutex_);
    appleId_ = appleId;
    gpDeviceId_ = gpDeviceId;
}

std::string ApiResourcesManager::sessionStatus() const
{
    return persistentSettings_.sessionStatus();
}

std::string ApiResourcesManager::portMap() const
{
    return persistentSettings_.portMap();
}

std::string ApiResourcesManager::locations() const
{
    return persistentSettings_.locations();
}

std::string ApiResourcesManager::staticIps() const
{
    return persistentSettings_.staticIps();
}

std::string ApiResourcesManager::serverCredentialsOvpn() const
{
    return persistentSettings_.serverCredentialsOvpn();
}

std::string ApiResourcesManager::serverCredentialsIkev2() const
{
    return persistentSettings_.serverCredentialsIkev2();
}

std::string ApiResourcesManager::serverConfigs() const
{
    return persistentSettings_.serverConfigs();
}

std::string ApiResourcesManager::notifications() const
{
    return persistentSettings_.notifications();
}

std::string ApiResourcesManager::checkUpdate() const
{
    std::lock_guard locker(mutex_);
    return checkUpdate_;
}

void ApiResourcesManager::setUpdateIntervals(int sessionInDisconnectedStateMs, int sessionInConnectedStateMs,
                                             int locationsMs, int staticIpsMs, int serverConfigsAndCredentialsMs,
                                             int portMapMs, int notificationsMs, int checkUpdateMs)
{
    std::lock_guard locker(mutex_);
    sessionInDisconnectedStateMs_ = sessionInDisconnectedStateMs;
    sessionInConnectedStateMs_ = sessionInConnectedStateMs;
    locationsMs_ = locationsMs;
    staticIpsMs_ = staticIpsMs;
    serverConfigsAndCredentialsMs_ = serverConfigsAndCredentialsMs;
    portMapMs_ = portMapMs;
    notificationsMs_ = notificationsMs;
    checkUpdateMs_ = checkUpdateMs;
}

void ApiResourcesManager::handleLoginOrSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    if (serverApiRetCode == ServerApiRetCode::kSuccess)  {
        std::unique_ptr<SessionStatus> ss(SessionStatus::createFromJson(jsonData));
        if (ss) {
            if (ss->errorCode() == SessionErrorCode::kSuccess) {
                sessionStatus_ = std::move(ss);
                persistentSettings_.setSessionStatus(jsonData);
                if (!sessionStatus_->authHash().empty()) {
                    persistentSettings_.setAuthHash(sessionStatus_->authHash());
                }
                lastUpdateTimeMs_[RequestType::kSessionStatus] = { steady_clock::now(), true };
                updateSessionStatus();
                checkForReadyLogin();
                fetchAll();

                // start the update timer
                fetchTimer_.async_wait(std::bind(&ApiResourcesManager::onFetchTimer, this, std::placeholders::_1));

            } else if (ss->errorCode() == SessionErrorCode::kBadUsername) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kBadUsername, ss->errorMessage());
            } else if (ss->errorCode() == SessionErrorCode::kMissingCode2FA) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kMissingCode2fa, ss->errorMessage());
            } else if (ss->errorCode() == SessionErrorCode::kBadCode2FA) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kBadCode2fa, ss->errorMessage());
            } else if (ss->errorCode() == SessionErrorCode::kAccountDisabled) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kAccountDisabled, ss->errorMessage());

            } else if (ss->errorCode() == SessionErrorCode::kSessionInvalid) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kSessionInvalid, ss->errorMessage());
            } else if (ss->errorCode() == SessionErrorCode::kRateLimited) {
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kRateLimited, ss->errorMessage());
            } else {
                assert(false);
                callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kNoApiConnectivity, ss->errorMessage());
            }
        } else {
            callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kIncorrectJson, std::string());
        }
    } else if (serverApiRetCode == ServerApiRetCode::kNoNetworkConnection) {
        callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kNoConnectivity, std::string());
    } else if (serverApiRetCode == ServerApiRetCode::kIncorrectJson) {
        callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kIncorrectJson, std::string());
    } else if (serverApiRetCode == ServerApiRetCode::kFailoverFailed) {
        callback_->call(ApiResourcesManagerNotification::kLoginFailed, LoginResult::kNoApiConnectivity, std::string());
    } else {
        assert(false);
    }
}

void ApiResourcesManager::checkForReadyLogin()
{
    if (!persistentSettings_.authHash().empty() &&
        !persistentSettings_.sessionStatus().empty() &&
        !persistentSettings_.locations().empty() &&
        !persistentSettings_.serverCredentialsOvpn().empty() &&
        !persistentSettings_.serverCredentialsIkev2().empty() &&
        !persistentSettings_.serverConfigs().empty() &&
        !persistentSettings_.portMap().empty() &&
        !persistentSettings_.staticIps().empty() &&
        !persistentSettings_.notifications().empty()) {

        if (!isLoginOkEmitted_) {
            isLoginOkEmitted_ = true;
            callback_->call(ApiResourcesManagerNotification::kLoginOk, LoginResult::kSuccess, std::string());
        }
    }
}

void ApiResourcesManager::checkForServerCredentialsFetchFinished()
{
    if (isFetchingServerCredentials_ && isOpenVpnCredentialsReceived_ && isIkev2CredentialsReceived_ && isServerConfigsReceived_) {
        isFetchingServerCredentials_ = false;
        callback_->call(ApiResourcesManagerNotification::kServerCredentialsUpdated, LoginResult::kSuccess, std::string());
    }
}

void ApiResourcesManager::fetchAll()
{
    // fetch session
    if (connectState_.isVPNConnected()) {
        // every 1 min in the connected state
        if (isTimeoutForRequest(RequestType::kSessionStatus, sessionInConnectedStateMs_))
            fetchSession(persistentSettings_.authHash());
    } else {
        // every 1 hour in the disconnected state
        if (isTimeoutForRequest(RequestType::kSessionStatus, sessionInDisconnectedStateMs_))
            fetchSession(persistentSettings_.authHash());
    }

    // fetch locations every 24 hours
    if (isTimeoutForRequest(RequestType::kLocations, locationsMs_))
        fetchLocations();

    // fetch static ips every 24 hours
    if (isTimeoutForRequest(RequestType::kStaticIps, staticIpsMs_))
        fetchStaticIps(persistentSettings_.authHash());

    // fetch server configs every 24 hours
    if (isTimeoutForRequest(RequestType::kServerConfigs, serverConfigsAndCredentialsMs_))
        fetchServerConfigs(persistentSettings_.authHash());

    // fetch server credentials every 24 hours
    if (isTimeoutForRequest(RequestType::kServerCredentialsOpenVPN, serverConfigsAndCredentialsMs_))
        fetchServerCredentialsOpenVpn(persistentSettings_.authHash());
    if (isTimeoutForRequest(RequestType::kServerCredentialsIkev2, serverConfigsAndCredentialsMs_))
        fetchServerCredentialsIkev2(persistentSettings_.authHash());

    // fetch portmap every 24 hours
    if (isTimeoutForRequest(RequestType::kPortMap, portMapMs_))
        fetchPortMap(persistentSettings_.authHash());

    // fetch notifications every 1 hour
    if (isTimeoutForRequest(RequestType::kNotifications, notificationsMs_))
        fetchNotifications(persistentSettings_.authHash());

    // fetch updates every 24 hour
    if (isCheckUpdateDataSet_ && isTimeoutForRequest(RequestType::kCheckUpdate, checkUpdateMs_))
        fetchCheckUpdate();

}

void ApiResourcesManager::fetchSession(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kSessionStatus) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kSessionStatus] = serverAPI_->session(authHash, appleId_, gpDeviceId_, std::bind(&ApiResourcesManager::onSessionAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchLocations()
{
    if (requestsInProgress_.find(RequestType::kLocations) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kLocations] = serverAPI_->serverLocations("en", sessionStatus_->revisionHash(), sessionStatus_->isPremium(), sessionStatus_->alcList(),
                                                                               std::bind(&ApiResourcesManager::onServerLocationsAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchStaticIps(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kStaticIps) != requestsInProgress_.end())
        return;

    if (sessionStatus_->staticIpsCount() > 0) {

        using namespace std::placeholders;
        requestsInProgress_[RequestType::kStaticIps] = serverAPI_->staticIps(authHash, std::bind(&ApiResourcesManager::onStaticIpsAnswer, this, _1, _2));
    } else {
        // We can't use an empty string because the initialization logic relies on comparison with the empty string
        // So use empty json object
        persistentSettings_.setStaticIps("{}");
        lastUpdateTimeMs_[RequestType::kStaticIps] = { steady_clock::now(), true };
        checkForReadyLogin();
        if (isLoginOkEmitted_)
            callback_->call(ApiResourcesManagerNotification::kStaticIpsUpdated, LoginResult::kSuccess, std::string());
        else
            checkForReadyLogin();
    }
}

void ApiResourcesManager::fetchServerConfigs(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kServerConfigs) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kServerConfigs] = serverAPI_->serverConfigs(authHash,
                                                                                 std::bind(&ApiResourcesManager::onServerConfigsAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchServerCredentialsOpenVpn(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kServerCredentialsOpenVPN) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kServerCredentialsOpenVPN] = serverAPI_->serverCredentials(authHash, true,
                                                                     std::bind(&ApiResourcesManager::onServerCredentialsOpenVpnAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchServerCredentialsIkev2(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kServerCredentialsIkev2) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kServerCredentialsIkev2] = serverAPI_->serverCredentials(authHash, false,
                                                                    std::bind(&ApiResourcesManager::onServerCredentialsIkev2Answer, this, _1, _2));

}

void ApiResourcesManager::fetchPortMap(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kPortMap) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kPortMap] = serverAPI_->portMap(authHash, 6, std::vector<std::string>(),
                                                                     std::bind(&ApiResourcesManager::onPortMapAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchNotifications(const std::string &authHash)
{
    if (requestsInProgress_.find(RequestType::kNotifications) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kNotifications] = serverAPI_->notifications(authHash, pcpidNotifications_,
                                                                                 std::bind(&ApiResourcesManager::onNotificationsAnswer, this, _1, _2));
}

void ApiResourcesManager::fetchCheckUpdate()
{
    if (requestsInProgress_.find(RequestType::kCheckUpdate) != requestsInProgress_.end())
        return;

    using namespace std::placeholders;
    requestsInProgress_[RequestType::kCheckUpdate] = serverAPI_->checkUpdate(checkUpdateData_.channel, checkUpdateData_.appVersion, checkUpdateData_.appBuild,
                                                                               checkUpdateData_.osVersion, checkUpdateData_.osBuild,
                                                                                 std::bind(&ApiResourcesManager::onCheckUpdateAnswer, this, _1, _2));
}

void ApiResourcesManager::updateSessionStatus()
{
    assert(sessionStatus_);

    if (prevSessionStatus_) {

        if (prevSessionStatus_->isPremium() != sessionStatus_->isPremium() ||
            prevSessionStatus_->status() != sessionStatus_->status() ||
            prevSessionStatus_->rebill() != sessionStatus_->rebill() ||
            prevSessionStatus_->billingPlanId() != sessionStatus_->billingPlanId() ||
            prevSessionStatus_->premiumExpiredDate() != sessionStatus_->premiumExpiredDate() ||
            prevSessionStatus_->trafficMax() != sessionStatus_->trafficMax() ||
            prevSessionStatus_->username() != sessionStatus_->username() ||
            prevSessionStatus_->userId() != sessionStatus_->userId() ||
            prevSessionStatus_->email() != sessionStatus_->email() ||
            prevSessionStatus_->emailStatus() != sessionStatus_->emailStatus() ||
            prevSessionStatus_->staticIpsCount() != sessionStatus_->staticIpsCount() ||
            prevSessionStatus_->alcList() != sessionStatus_->alcList() ||
            prevSessionStatus_->lastResetDate() != sessionStatus_->lastResetDate())
        {
            spdlog::info("update session status (changed since last call)");
            sessionStatus_->debugLog();
        }


        if (prevSessionStatus_->revisionHash() != sessionStatus_->revisionHash() || prevSessionStatus_->isPremium() != sessionStatus_->isPremium() ||
            prevSessionStatus_->billingPlanId() != sessionStatus_->billingPlanId() ||
            prevSessionStatus_->alcList() != sessionStatus_->alcList() || (prevSessionStatus_->status() != 1 && sessionStatus_->status() == 1)) {

            fetchLocations();
        }

        if (prevSessionStatus_->revisionHash() != sessionStatus_->revisionHash() || prevSessionStatus_->staticIpsCount() != sessionStatus_->staticIpsCount() ||
            sessionStatus_->isContainsStaticDeviceId(Settings::instance().deviceId()) ||
            prevSessionStatus_->isPremium() != sessionStatus_->isPremium() ||
            prevSessionStatus_->billingPlanId() != sessionStatus_->billingPlanId()) {

            fetchStaticIps(persistentSettings_.authHash());
        }

        if (prevSessionStatus_->isPremium() != sessionStatus_->isPremium() || prevSessionStatus_->billingPlanId() != sessionStatus_->billingPlanId())  {
            fetchServerCredentialsOpenVpn(persistentSettings_.authHash());
            fetchServerCredentialsIkev2(persistentSettings_.authHash());
            fetchNotifications(persistentSettings_.authHash());
        }

        if (prevSessionStatus_->status() == 2 && sessionStatus_->status() == 1) {
            fetchServerCredentialsOpenVpn(persistentSettings_.authHash());
            fetchServerCredentialsIkev2(persistentSettings_.authHash());
        }
    } else {
        spdlog::info("update session status (changed since last call)");
        sessionStatus_->debugLog();
    }

    prevSessionStatus_ = std::make_unique<SessionStatus>(sessionStatus_.get());
    if (isLoginOkEmitted_)
        callback_->call(ApiResourcesManagerNotification::kSessionUpdated, LoginResult::kSuccess, std::string());
}

void ApiResourcesManager::onFetchTimer(const boost::system::error_code &err)
{
    if (err)
        return;

    std::lock_guard locker(mutex_);

    if (!persistentSettings_.authHash().empty()) {
        fetchAll();
    } else  {
        spdlog::error("ApiResourcesManager::onFetchTimer, authHash is empty although it shouldn't");
        assert(false);
    }

    // repeat the update timer
    fetchTimer_.expires_from_now(boost::asio::chrono::seconds(1));
    fetchTimer_.async_wait(std::bind(&ApiResourcesManager::onFetchTimer, this, std::placeholders::_1));
}

void ApiResourcesManager::onInitialSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    requestsInProgress_.erase(RequestType::kSessionStatus);
    if (serverApiRetCode == ServerApiRetCode::kNetworkError) {
        // repeat the request
        boost::asio::post(io_context_, [this] {
            loginWithAuthHash();
        });
    } else {
        handleLoginOrSessionAnswer(serverApiRetCode, jsonData);
    }
}

void ApiResourcesManager::onLoginAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData, const std::string &username, const std::string &password, const std::string &code2fa)
{
    std::lock_guard locker(mutex_);
    requestsInProgress_.erase(RequestType::kSessionStatus);
    if (serverApiRetCode == ServerApiRetCode::kNetworkError) {
        // repeat the request
        boost::asio::post(io_context_, [this, username, password, code2fa] {
            login(username, password, code2fa);
        });
    } else {
        handleLoginOrSessionAnswer(serverApiRetCode, jsonData);
    }
}

void ApiResourcesManager::onSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        std::unique_ptr<SessionStatus> ss(SessionStatus::createFromJson(jsonData));
        if (ss) {
            if (ss->errorCode() == SessionErrorCode::kSuccess) {
                sessionStatus_ = std::move(ss);
                persistentSettings_.setSessionStatus(jsonData);
                updateSessionStatus();
            } else if (ss->errorCode() == SessionErrorCode::kSessionInvalid) {
                callback_->call(ApiResourcesManagerNotification::kSessionDeleted, LoginResult::kSuccess, std::string());
            }
        }
    }
    lastUpdateTimeMs_[RequestType::kSessionStatus] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kSessionStatus);
}

void ApiResourcesManager::onServerLocationsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setLocations(jsonData);
        if (isLoginOkEmitted_)
            callback_->call(ApiResourcesManagerNotification::kLocationsUpdated, LoginResult::kSuccess, std::string());
        else
            checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kLocations] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kLocations);
}

void ApiResourcesManager::onStaticIpsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setStaticIps(jsonData);
        checkForReadyLogin();
        if (isLoginOkEmitted_)
            callback_->call(ApiResourcesManagerNotification::kStaticIpsUpdated, LoginResult::kSuccess, std::string());
        else
            checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kStaticIps] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kStaticIps);
}

void ApiResourcesManager::onServerConfigsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setServerConfigs(jsonData);
        isServerConfigsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kServerConfigs] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kServerConfigs);

}

void ApiResourcesManager::onServerCredentialsOpenVpnAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);

    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setServerCredentialsOvpn(jsonData);
        isOpenVpnCredentialsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kServerCredentialsOpenVPN] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kServerCredentialsOpenVPN);
}

void ApiResourcesManager::onServerCredentialsIkev2Answer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);

    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setServerCredentialsIkev2(jsonData);
        isIkev2CredentialsReceived_ = true;
        checkForServerCredentialsFetchFinished();
        checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kServerCredentialsIkev2] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kServerCredentialsIkev2);
}

void ApiResourcesManager::onPortMapAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);

    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setPortMap(jsonData);
        checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kPortMap] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kPortMap);
}

void ApiResourcesManager::onNotificationsAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);

    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        persistentSettings_.setNotifications(jsonData);
        if (isLoginOkEmitted_)
            callback_->call(ApiResourcesManagerNotification::kNotificationsUpdated, LoginResult::kSuccess, std::string());
        else
            checkForReadyLogin();
    }
    lastUpdateTimeMs_[RequestType::kNotifications] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kNotifications);
}

void ApiResourcesManager::onCheckUpdateAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);

    if (serverApiRetCode == ServerApiRetCode::kSuccess) {
        checkUpdate_ = jsonData;
        callback_->call(ApiResourcesManagerNotification::kCheckUpdate, LoginResult::kSuccess, std::string());
    }
    lastUpdateTimeMs_[RequestType::kCheckUpdate] = { steady_clock::now(), serverApiRetCode == ServerApiRetCode::kSuccess };
    requestsInProgress_.erase(RequestType::kCheckUpdate);
}

void ApiResourcesManager::onDeleteSessionAnswer(ServerApiRetCode serverApiRetCode, const std::string &jsonData)
{
    std::lock_guard locker(mutex_);
    spdlog::info("ApiResourcesManager::onDeleteSessionAnswer retCode: {}", (int)serverApiRetCode);
    callback_->call(ApiResourcesManagerNotification::kLogoutFinished, LoginResult::kSuccess, std::string());
}

bool ApiResourcesManager::isTimeoutForRequest(RequestType requestType, int timeout)
{
    auto it = lastUpdateTimeMs_.find(requestType);
    if (it == lastUpdateTimeMs_.end())
        return true;

    if (it->second.isRequestSuccess) {
        if (utils::since(it->second.updateTime).count() > timeout)
            return true;
    } else {
        if (utils::since(it->second.updateTime).count() > kDelayBetweenFailedRequests)
            return true;
    }

    return false;
}

void ApiResourcesManager::clearValues()
{
    isFetchingServerCredentials_ = false;
    isLoginOkEmitted_ = false;
    sessionStatus_.reset();
    prevSessionStatus_.reset();
    checkUpdate_.clear();
    startLoginTime_.reset();
    lastUpdateTimeMs_.clear();
    persistentSettings_.setAuthHash(std::string());
    persistentSettings_.setSessionStatus(std::string());
    persistentSettings_.setLocations(std::string());
    persistentSettings_.setServerCredentialsOvpn(std::string());
    persistentSettings_.setServerCredentialsIkev2(std::string());
    persistentSettings_.setServerConfigs(std::string());
    persistentSettings_.setPortMap(std::string());
    persistentSettings_.setStaticIps(std::string());
    persistentSettings_.setNotifications(std::string());
}

} // namespace wsnet
