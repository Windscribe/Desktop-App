#include "serverapi.h"
#include <spdlog/spdlog.h>
#include "serverapi_impl.h"
#include "requestsfactory.h"
#include "settings.h"

namespace wsnet {

ServerAPI::ServerAPI(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer,
                     PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    io_context_(io_context),
    persistentSettings_(persistentSettings),
    advancedParameters_(advancedParameters),
    connectState_(connectState)
{
    impl_ = std::make_unique<ServerAPI_impl>(httpNetworkManager, failoverContainer, persistentSettings_, advancedParameters, connectState);
    subscriberId_ = connectState_.subscribeConnectedToVpnState(std::bind(&ServerAPI::onVPNConnectStateChanged, this, std::placeholders::_1));
}

ServerAPI::~ServerAPI()
{
    connectState_.unsubscribeConnectedToVpnState(subscriberId_);
}

void ServerAPI::setApiResolutionsSettings(bool isAutomatic, std::string manualAddress)
{
    boost::asio::post(io_context_, [this, isAutomatic, manualAddress] {
        impl_->setApiResolutionsSettings(isAutomatic, manualAddress);
    });
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    boost::asio::post(io_context_, [this, bIgnore] {
        impl_->setIgnoreSslErrors(bIgnore);
    });
}

void ServerAPI::resetFailover()
{
    boost::asio::post(io_context_, [this] {
        impl_->resetFailover();
    });
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::setTryingBackupEndpointCallback(WSNetTryingBackupEndpointCallback tryingBackupEndpointCallback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetTryingBackupEndpointCallback>>(tryingBackupEndpointCallback);
    boost::asio::post(io_context_, [this, cancelableCallback] {
        impl_->setTryingBackupEndpointCallback(cancelableCallback);
    });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::login(const std::string &username, const std::string &password, const std::string &code2fa, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::login(username, password, code2fa, Settings::instance().sessionTypeId(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::session(const std::string &authHash, const std::string &appleId, const std::string &gpDeviceId, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::session(authHash, appleId, gpDeviceId, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::claimVoucherCode(const std::string &authHash, const std::string &voucherCode, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::claimVoucherCode(authHash, voucherCode, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::deleteSession(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::deleteSession(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverLocations(const std::string &language, const std::string &revision, bool isPro, const std::vector<std::string> &alcList, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverLocations(persistentSettings_, language, revision, isPro, alcList,
                                                             connectState_, advancedParameters_, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverCredentials(const std::string &authHash, bool isOpenVpnProtocol, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverCredentials(authHash, isOpenVpnProtocol, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::serverConfigs(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::serverConfigs(authHash, Settings::instance().openVpnVersion(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::portMap(const std::string &authHash, std::uint32_t version, const std::vector<std::string> &forceProtocols, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::portMap(authHash, version, forceProtocols, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::recordInstall(bool isDesktop, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::recordInstall(isDesktop, Settings::instance().basePlatform(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::addEmail(const std::string &authHash, const std::string &email, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::addEmail(authHash, email, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::confirmEmail(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::confirmEmail(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::signup(const std::string &username, const std::string &password, const std::string &referringUsername, const std::string &email, const std::string &voucherCode, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::signup(username, password, referringUsername, email, Settings::instance().sessionTypeId(), voucherCode, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::webSession(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::webSession(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::checkUpdate(UpdateChannel updateChannel, const std::string &appVersion, const std::string &appBuild,
                                                                const std::string &osVersion, const std::string &osBuild, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::checkUpdate(updateChannel, appVersion, appBuild, osVersion, osBuild, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::debugLog(const std::string &username, const std::string &strLog, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::debugLog(username, strLog, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::speedRating(const std::string &authHash, const std::string &hostname, const std::string &ip, std::int32_t rating, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::speedRating(authHash, hostname, ip, rating, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::staticIps(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::staticIps(authHash, Settings::instance().basePlatform(), Settings::instance().deviceId(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::pingTest(std::uint32_t timeoutMs, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::pingTest(timeoutMs, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::notifications(const std::string &authHash, const std::string &pcpid, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::notifications(authHash, pcpid, Settings::instance().language(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::getRobertFilters(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::getRobertFilters(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::setRobertFilter(const std::string &authHash, const std::string &id, std::int32_t status, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::setRobertFilter(authHash, id, status, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::syncRobert(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::syncRobert(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::wgConfigsInit(const std::string &authHash, const std::string &clientPublicKey, bool deleteOldestKey, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::wgConfigsInit(authHash, clientPublicKey, deleteOldestKey, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::wgConfigsConnect(const std::string &authHash, const std::string &clientPublicKey, const std::string &hostname, const std::string &deviceId, const std::string &wgTtl, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::wgConfigsConnect(authHash, clientPublicKey, hostname, deviceId, wgTtl, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::myIP(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::myIP(cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::mobileBillingPlans(const std::string &authHash, const std::string &mobilePlanType, const std::string &promo, int version, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::mobileBillingPlans(authHash, mobilePlanType, promo, version, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::sendPayment(const std::string &authHash, const std::string &appleID, const std::string &appleData, const std::string &appleSIG, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::sendPayment(authHash, appleID, appleData, appleSIG, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::verifyPayment(const std::string &authHash, const std::string &purchaseToken, const std::string &gpPackageName, const std::string &gpProductId, const std::string &type, const std::string &amazonUserId, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::verifyPayment(authHash, purchaseToken, gpPackageName, gpProductId, type, amazonUserId, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::postBillingCpid(const std::string &authHash, const std::string &payCpid, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::postBillingCpid(authHash, payCpid, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::getXpressLoginCode(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::getXpressLoginCode(cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::verifyXpressLoginCode(const std::string &xpressCode, const std::string &sig, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::verifyXpressLoginCode(xpressCode, sig, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::sendSupportTicket(const std::string &supportEmail, const std::string &supportName,
                                                                      const std::string &supportSubject, const std::string &supportMessage,
                                                                      const std::string &supportCategory, const std::string &type, const std::string &channel, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::sendSupportTicket(supportEmail, supportName, supportSubject, supportMessage, supportCategory, type, channel, Settings::instance().basePlatform(), cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::regToken(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::regToken(cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::signupUsingToken(const std::string &token, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::signupUsingToken(token, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::claimAccount(const std::string &authHash, const std::string &username, const std::string &password, const std::string &email, const std::string &voucherCode, const std::string &claimAccount, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::claimAccount(authHash, username, password, email, voucherCode, claimAccount, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::shakeData(const std::string &authHash, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::shakeData(authHash, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::recordShakeForDataScore(const std::string &authHash, const std::string &score, const std::string &signature, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::recordShakeForDataScore(authHash, Settings::instance().basePlatform(), score, signature, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::verifyTvLoginCode(const std::string &authHash, const std::string &xpressCode, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::verifyTvLoginCode(authHash, xpressCode, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> ServerAPI::cancelAccount(const std::string &authHash, const std::string &password, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::cancelAccount(authHash, password, cancelableCallback);
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

void ServerAPI::onVPNConnectStateChanged(bool isConnected)
{
    boost::asio::post(io_context_, [this, isConnected] {
        impl_->setIsConnectedToVpnState(isConnected);
    });
}

} // namespace wsnet
