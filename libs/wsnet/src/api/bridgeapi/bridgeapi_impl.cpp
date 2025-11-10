#include "bridgeapi_impl.h"

#include "bridgeapi_utils.h"
#include "bridgeapi_requestsfactory.h"
#include "bridgetokenrequest.h"
#include "settings.h"
#include "utils/cancelablecallback.h"
#include "utils/wsnet_logger.h"
#include "utils/requesterror.h"

namespace wsnet {

BridgeAPI_impl::BridgeAPI_impl(WSNetHttpNetworkManager *httpNetworkManager, PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters),
    connectState_(connectState),
    persistentSettings_(persistentSettings)
{
    sessionTokens_ = persistentSettings_.sessionTokens();
}

BridgeAPI_impl::~BridgeAPI_impl()
{
    for (auto &it : activeHttpRequests_) {
        it.second.asyncCallback_->cancel();
    }

    // Clean up any queued pinIp request
    if (queuedPinIpRequest_) {
        queuedPinIpRequest_->setRetCode(ApiRetCode::kNoToken);
        queuedPinIpRequest_->callCallback();
        queuedPinIpRequest_.reset();
    }
}

void BridgeAPI_impl::setConnectedState(bool isConnected)
{
    if (isConnected && !isConnected_) {
        std::string currentToken;
        auto token = sessionToken();
        if (token) {
            if (token->second > 0) {
                // We have a valid token with an expiry time, check if it's expired
                auto now = std::chrono::system_clock::now();
                auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                if (nowTimestamp >= token->second) {
                    g_logger->info("Removing expired session token for host '{}'", currentHost_);
                    sessionTokens_.erase(currentHost_);
                    persistentSettings_.setSessionTokens(sessionTokens_);
                } else {
                    // Token is not expired, set expiry time to 0 since it's valid until disconnection
                    currentToken = token->first;
                    sessionTokens_[currentHost_].second = 0;
                    persistentSettings_.setSessionTokens(sessionTokens_);
                }
            } else {
                currentToken = token->first;
            }
        }

        g_logger->info("BridgeAPI_impl::setConnectedState, fetching token for host '{}'{}", currentHost_, currentToken.empty() ? "" : " with cached token");
        isFetchingToken_ = true;
        auto request = std::unique_ptr<BaseRequest>(bridgeapi_requests_factory::fetchToken(currentToken, nullptr));
        executeRequestImpl(std::move(request));
    } else if (!isConnected) {
        setSessionTokenExpiry();
        isFetchingToken_ = false;

        // Clear any queued pinIp request since we're disconnected
        if (queuedPinIpRequest_) {
            g_logger->info("Clearing queued pinIp request due to disconnection");
            queuedPinIpRequest_->setRetCode(ApiRetCode::kNoNetworkConnection);
            queuedPinIpRequest_->callCallback();
            queuedPinIpRequest_.reset();
        }

        // Notify that the API is no longer available
        if (apiAvailableCallback_) {
            apiAvailableCallback_(false);
        }
    }

    isConnected_ = isConnected;
}

void BridgeAPI_impl::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
    g_logger->info("BridgeAPI_impl::setIgnoreSslErrors, {}", bIgnore);
}

// execute request if the failover detected or queue
void BridgeAPI_impl::executeRequest(std::unique_ptr<BaseRequest> request)
{
    // if request already canceled do nothing
    if (request->isCanceled()) {
        return;
    }

    // check if we are online
    if (!connectState_.isOnline() || !isConnected_) {
        request->setRetCode(ApiRetCode::kNoNetworkConnection);
        request->callCallback();
        return;
    }

    // Check if we need a token and don't have one
    if (!sessionToken()) {
        request->setRetCode(ApiRetCode::kNoToken);
        request->callCallback();
        return;
    }

    // in the connected mode always use the primary domain
    executeRequestImpl(std::move(request));
}

void BridgeAPI_impl::executeRequestImpl(std::unique_ptr<BaseRequest> request)
{
    using namespace std::placeholders;

    g_logger->info("BridgeAPI_impl::executeRequestImpl, bIgnoreSslErrors_: {}", bIgnoreSslErrors_);
    auto httpRequest = serverapi_utils::createHttpRequest(httpNetworkManager_, Settings::instance().bridgeApiAddress(), request.get(), bIgnoreSslErrors_, advancedParameters_->isAPIExtraTLSPadding());
    httpRequest->setIsDebugLogCurlError(true);
    std::uint64_t requestId = curUniqueId_++;
    auto asyncCallback_ = httpNetworkManager_->executeRequestEx(httpRequest, requestId, std::bind(&BridgeAPI_impl::onHttpNetworkRequestFinished, this, _1, _2, _3, _4),
                                                                std::bind(&BridgeAPI_impl::onHttpNetworkRequestProgressCallback, this, _1, _2, _3));
    HttpRequestInfo hti { std::move(request), asyncCallback_};
    activeHttpRequests_[requestId] = std::move(hti);
}

void BridgeAPI_impl::setErrorCodeAndEmitRequestFinished(BaseRequest *request, ApiRetCode retCode)
{
    request->setRetCode(retCode);
    request->callCallback();
}

void BridgeAPI_impl::onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data)
{
    auto it = activeHttpRequests_.find(requestId);
    assert(it != activeHttpRequests_.end());

    if (it->second.request->isCanceled()) {
        activeHttpRequests_.erase(it);
        return;
    }

    // Extract HTTP response code from the error object
    int httpResponseCode = 0;
    auto requestError = std::dynamic_pointer_cast<RequestError>(error);
    if (requestError) {
        httpResponseCode = requestError->httpResponseCode();
    }

    // Check if this is a token fetch request
    auto tokenRequest = dynamic_cast<BridgeTokenRequest*>(it->second.request.get());
    bool isTokenRequest = (tokenRequest != nullptr);

    // If this is a token request and we're no longer connected, ignore the result
    if (isTokenRequest && !isConnected_) {
        g_logger->info("Token request finished but no longer connected, ignoring result");
        activeHttpRequests_.erase(it);
        return;
    }

    if (error->isSuccess() && httpResponseCode == 200) {
        if (advancedParameters_->isLogApiResponce()) {
            g_logger->info("API request {} finished", it->second.request->name());
            g_logger->info("{}", data);
        }
        it->second.request->handle(data);
        if (it->second.request->retCode() == ApiRetCode::kSuccess) {
            if (isTokenRequest) {
                isFetchingToken_ = false;
                std::string newToken = tokenRequest->getSessionToken();
                sessionTokens_[currentHost_] = {newToken, 0};
                g_logger->info("Successfully received session token for host '{}'", currentHost_);

                persistentSettings_.setSessionTokens(sessionTokens_);

                // Notify that the API is now available
                if (apiAvailableCallback_) {
                    apiAvailableCallback_(true);
                }

                // Execute any queued pinIp request now that we have a token
                if (queuedPinIpRequest_) {
                    g_logger->info("Executing queued pinIp request with fresh token");
                    // Update the token on the request before executing
                    auto bridgeRequest = dynamic_cast<BridgeAPIRequest*>(queuedPinIpRequest_.get());
                    if (bridgeRequest) {
                        auto token = sessionToken();
                        if (token) {
                            bridgeRequest->setSessionToken(token->first);
                        }
                    }
                    executeRequest(std::move(queuedPinIpRequest_));
                    queuedPinIpRequest_.reset();
                }
            } else {
                it->second.request->callCallback();
            }
            activeHttpRequests_.erase(it);
            return;
        }
    }

    // Failure
    if (isTokenRequest) {
        auto token = sessionToken();
        if (token && error->isSuccess()) {
            // We have an existing token and server told us it's no good (i.e. httpResponseCode != 200), try getting a new one
            clearSessionToken();
            g_logger->info("Token request failed {} (HTTP response code: {}), clearing cached token for host '{}' and trying again", error->toString(), httpResponseCode, currentHost_);
            auto request = std::unique_ptr<BaseRequest>(bridgeapi_requests_factory::fetchToken("", nullptr));
            executeRequestImpl(std::move(request));
        } else {
            isFetchingToken_ = false;
            g_logger->info("Token request failed: {} (HTTP response code: {})", error->toString(), httpResponseCode);
        }

        // Clear any queued pinIp request since token fetch failed
        if (queuedPinIpRequest_) {
            g_logger->info("Clearing queued pinIp request due to token fetch failure");
            queuedPinIpRequest_->setRetCode(ApiRetCode::kNoToken);
            queuedPinIpRequest_->callCallback();
            queuedPinIpRequest_.reset();
        }
    } else {
        g_logger->info("API request {} failed with error = {} (HTTP response code: {})", it->second.request->name(), error->toString(), httpResponseCode);
        setErrorCodeAndEmitRequestFinished(it->second.request.get(), it->second.request->retCode() == ApiRetCode::kSuccess ? ApiRetCode::kBridgeApiError : it->second.request->retCode());
    }
    activeHttpRequests_.erase(it);
}

void BridgeAPI_impl::onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    auto it = activeHttpRequests_.find(requestId);
    assert(it != activeHttpRequests_.end());
    if (it->second.request->isCanceled()) {
        it->second.asyncCallback_->cancel();
        activeHttpRequests_.erase(it);
    }
}

const std::pair<std::string, std::int64_t> *BridgeAPI_impl::sessionToken() const
{
    auto it = sessionTokens_.find(currentHost_);
    if (it != sessionTokens_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool BridgeAPI_impl::hasSessionToken() const
{
    return sessionToken() != nullptr;
}

void BridgeAPI_impl::setApiAvailableCallback(WSNetApiAvailableCallback callback)
{
    apiAvailableCallback_ = callback;
}

void BridgeAPI_impl::pinIp(std::unique_ptr<BaseRequest> request)
{
    // Store the request to be executed when session token is available
    queuedPinIpRequest_ = std::move(request);

    // If we are connected, and not currently fetching a new token, execute immediately
    if (isConnected_ && !isFetchingToken_) {
        executeRequest(std::move(queuedPinIpRequest_));
        queuedPinIpRequest_.reset();
    }

    // Otherwise, the request will be executed when the token fetch completes, or cancelled if the token request fails
}

void BridgeAPI_impl::clearSessionToken()
{
    sessionTokens_.erase(currentHost_);
    persistentSettings_.setSessionTokens(sessionTokens_);
}

void BridgeAPI_impl::setCurrentHost(const std::string &host)
{
    g_logger->info("BridgeAPI_impl::setCurrentHost: '{}'", host);
    currentHost_ = host;
}

void BridgeAPI_impl::setSessionTokenExpiry()
{
    auto it = sessionTokens_.find(currentHost_);
    if (it != sessionTokens_.end()) {
        auto expiryTime = std::chrono::system_clock::now() + std::chrono::hours(24);
        auto expiryTimestamp = std::chrono::duration_cast<std::chrono::seconds>(expiryTime.time_since_epoch()).count();
        if (it->second.second == 0) {
            it->second.second = expiryTimestamp;
            persistentSettings_.setSessionTokens(sessionTokens_);
            g_logger->info("Set session token expiry to 24 hours from disconnect for host '{}'", currentHost_);
        }
    }
}

} // namespace wsnet
