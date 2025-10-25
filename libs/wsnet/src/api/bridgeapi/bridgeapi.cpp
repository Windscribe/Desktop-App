#include "bridgeapi.h"

#include <spdlog/spdlog.h>

#include "bridgeapi_impl.h"
#include "bridgeapi_request.h"
#include "bridgeapi_requestsfactory.h"
#include "settings.h"

namespace wsnet {

BridgeAPI::BridgeAPI(boost::asio::io_context &io_context, WSNetHttpNetworkManager *httpNetworkManager, PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    io_context_(io_context),
    persistentSettings_(persistentSettings),
    advancedParameters_(advancedParameters),
    connectState_(connectState)
{
    impl_ = std::make_unique<BridgeAPI_impl>(httpNetworkManager, persistentSettings_, advancedParameters, connectState);
    subscriberId_ = connectState_.subscribeConnectedToVpnState(std::bind(&BridgeAPI::onVPNConnectStateChanged, this, std::placeholders::_1));
}

BridgeAPI::~BridgeAPI()
{
    connectState_.unsubscribeConnectedToVpnState(subscriberId_);
}

void BridgeAPI::setConnectedState(bool isConnected)
{
    boost::asio::post(io_context_, [this, isConnected] {
        impl_->setConnectedState(isConnected);
    });
}

void BridgeAPI::setIgnoreSslErrors(bool bIgnore)
{
    boost::asio::post(io_context_, [this, bIgnore] {
        impl_->setIgnoreSslErrors(bIgnore);
    });
}

bool BridgeAPI::hasSessionToken() const
{
    return impl_->hasSessionToken();
}

void BridgeAPI::setApiAvailableCallback(WSNetApiAvailableCallback callback)
{
    boost::asio::post(io_context_, [this, callback] {
        impl_->setApiAvailableCallback(callback);
    });
}

std::shared_ptr<WSNetCancelableCallback> BridgeAPI::pinIp(const std::string &ip, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    // Use the bridge token from impl
    BaseRequest *request = bridgeapi_requests_factory::pinIp("", ip, cancelableCallback);
    // This request does not return any data
    request->setIgnoreJsonParse();
    static_cast<BridgeAPIRequest *>(request)->setSessionToken(impl_->sessionToken());
    boost::asio::post(io_context_, [this, request] { impl_->pinIp(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

std::shared_ptr<WSNetCancelableCallback> BridgeAPI::rotateIp(WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    // Use the bridge token from impl
    BaseRequest *request = bridgeapi_requests_factory::rotateIp("", cancelableCallback);
    // This request does not return any data
    request->setIgnoreJsonParse();
    static_cast<BridgeAPIRequest *>(request)->setSessionToken(impl_->sessionToken());
    boost::asio::post(io_context_, [this, request] { impl_->executeRequest(std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

void BridgeAPI::onVPNConnectStateChanged(bool isConnected)
{
    boost::asio::post(io_context_, [this, isConnected] {
        impl_->setConnectedState(isConnected);
    });
}

} // namespace wsnet
