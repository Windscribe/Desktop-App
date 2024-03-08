#include "serverapi.h"
#include <spdlog/spdlog.h>
#include "serverapi_impl.h"
#include "requestsfactory.h"

namespace wsnet {

ServerAPI::ServerAPI(BS::thread_pool &taskQueue, WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer,
                     const std::string &settings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    taskQueue_(taskQueue),
    settings_(settings),
    advancedParameters_(advancedParameters),
    connectState_(connectState)
{
    impl_ = std::make_unique<ServerAPI_impl>(httpNetworkManager, failoverContainer, settings_, advancedParameters, connectState);
    subscriberId_ = connectState_.subscribeConnectedToVpnState(std::bind(&ServerAPI::onVPNConnectStateChanged, this, std::placeholders::_1));
}

ServerAPI::~ServerAPI()
{
    connectState_.unsubscribeConnectedToVpnState(subscriberId_);
}

std::string ServerAPI::currentSettings()
{
    return settings_.getAsString();
}

void ServerAPI::setApiResolutionsSettings(bool isAutomatic, std::string manualAddress)
{
    taskQueue_.detach_task([this, isAutomatic, manualAddress] {
        impl_->setApiResolutionsSettings(isAutomatic, manualAddress);
    });
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    taskQueue_.detach_task([this, bIgnore] {
        impl_->setIgnoreSslErrors(bIgnore);
    });
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::setTryingBackupEndpointCallback(WSNetTryingBackupEndpointCallback tryingBackupEndpointCallback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetTryingBackupEndpointCallback>>(tryingBackupEndpointCallback);
    taskQueue_.detach_task([this, cancelableCallback] {
        impl_->setTryingBackupEndpointCallback(cancelableCallback);
    });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::login(const std::string &username, const std::string &password, const std::string &code2fa, WSNetRequestFinishedCallback callback)
{
    // For login only request, we reset a failover to the initial state
    taskQueue_.detach_task([this] {
        impl_->resetFailover();
    });

    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::login(username, password, code2fa, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::session(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::session(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::deleteSession(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::deleteSession(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverLocations(const std::string &language, const std::string &revision, bool isPro, const std::vector<std::string> &alcList, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverLocations(settings_, language, revision, isPro, alcList,
                                                             connectState_, advancedParameters_, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverCredentials(authHash, isOpenVpnProtocol, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverConfigs(const std::string &authHash, const std::string &ovpnVersion, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverConfigs(authHash, ovpnVersion, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::portMap(authHash, version, forceProtocols, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::recordInstall(const std::string &platform, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::recordInstall(platform, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::addEmail(const std::string &authHash, const std::string &email, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::addEmail(authHash, email, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::confirmEmail(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::confirmEmail(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::signup(const std::string &username, const std::string &password, const std::string &referringUsername, const std::string &email, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::signup(username, password, referringUsername, email, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::webSession(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::webSession(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::checkUpdate(UpdateChannel updateChannel, const std::string &appVersion, const std::string &appBuild,
                                                                const std::string &osVersion, const std::string &osBuild, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::checkUpdate(updateChannel, appVersion, appBuild, osVersion, osBuild, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::debugLog(const std::string &username, const std::string &strLog, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::debugLog(username, strLog, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip, std::int32_t rating, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::speedRating(authHash, hostname, ip, rating, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::staticIps(const std::string &authHash, const std::string &platform, const std::string &deviceId, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::staticIps(authHash, platform, deviceId, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::pingTest(std::uint32_t timeoutMs, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::pingTest(timeoutMs, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::notifications(const std::string &authHash, const std::string &pcpid, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::notifications(authHash, pcpid, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::getRobertFilters(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::getRobertFilters(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::setRobertFilter(const std::string &authHash, const std::string &id, std::int32_t status, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::setRobertFilter(authHash, id, status, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::syncRobert(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::syncRobert(authHash, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey, bool deleteOldestKey, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::wgConfigsInit(authHash, clientPublicKey, deleteOldestKey, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey, const std::string &hostname, const std::string &deviceId, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::wgConfigsConnect(authHash, clientPublicKey, hostname, deviceId, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::myIP(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::myIP(cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::mobileBillingPlans(const std::string &mobilePlanType, int version, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::mobileBillingPlans(mobilePlanType, version, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::verifyPayment(const std::string &authHash, const std::string &purchaseToken, const std::string &gpPackageName, const std::string &gpProductId, const std::string &type, const std::string &amazonUserId, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::verifyPayment(authHash, purchaseToken, gpPackageName, gpProductId, type, amazonUserId, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::postBillingCpid(const std::string &authHash, const std::string &payCpid, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::postBillingCpid(authHash, payCpid, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::getXpressLoginCode(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::getXpressLoginCode(cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::verifyXpressLoginCode(xpressCode, sig, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::sendSupportTicket(const std::string &supportEmail, const std::string &supportName,
                                                                      const std::string &supportSubject, const std::string &supportMessage,
                                                                      const std::string &supportCategory, const std::string &type, const std::string &channel,
                                                                      const std::string &platform, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::sendSupportTicket(supportEmail, supportName, supportSubject, supportMessage, supportCategory, type, channel, platform, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::regToken(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::regToken(cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::signupUsingToken(const std::string &token, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::signupUsingToken(token, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::claimAccount(const std::string &authHash, const std::string &username, const std::string &password, const std::string &email, const std::string &claimAccount, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::claimAccount(authHash, username, password, email, claimAccount, cancelableCallback);
    taskQueue_.detach_task([this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

void ServerAPI::onVPNConnectStateChanged(bool isConnected)
{
    taskQueue_.detach_task([this, isConnected] {
        impl_->setIsConnectedToVpnState(isConnected);
    });
}

} // namespace wsnet
