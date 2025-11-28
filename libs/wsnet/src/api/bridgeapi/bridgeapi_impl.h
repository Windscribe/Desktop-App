#pragma once

#include "WSNetBridgeAPI.h"
#include <mutex>
#include <queue>
#include <map>
#include <optional>
#include <atomic>
#include <chrono>
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include "../baserequest.h"
#include "bridgeapi_request.h"
#include "connectstate.h"
#include "utils/cancelablecallback.h"
#include "utils/persistentsettings.h"

namespace wsnet {

class BridgeAPI_impl
{
public:
    explicit BridgeAPI_impl(WSNetHttpNetworkManager *httpNetworkManager, PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState);
    virtual ~BridgeAPI_impl();

    void setConnectedState(bool isConnected);
    void setIgnoreSslErrors(bool bIgnore);

    const std::pair<std::string, std::int64_t> *sessionToken() const;
    bool hasSessionToken() const;
    void setApiAvailableCallback(WSNetApiAvailableCallback callback);
    void setCurrentHost(const std::string &host);

    void executeRequest(std::unique_ptr<BaseRequest> request);
    void pinIp(std::unique_ptr<BaseRequest> request);

private:
    WSNetHttpNetworkManager *httpNetworkManager_;
    WSNetAdvancedParameters *advancedParameters_;
    ConnectState &connectState_;

    std::uint64_t curUniqueId_ = 0;     // for generate unique identifiers for HTTP-requests

    PersistentSettings &persistentSettings_;    // The BridgeAPISettings class is protected by mutex, so it's thread-safe

    bool bIgnoreSslErrors_ = false;
    bool isConnected_ = false;
    bool isFetchingToken_ = false;
    std::map<std::string, std::pair<std::string, std::int64_t>> sessionTokens_;
    std::string currentHost_;
    std::unique_ptr<BaseRequest> queuedPinIpRequest_;
    WSNetApiAvailableCallback apiAvailableCallback_;

    struct HttpRequestInfo {
        std::unique_ptr<BaseRequest> request;
        std::shared_ptr<WSNetCancelableCallback> asyncCallback_;
    };
    std::map<std::uint64_t, HttpRequestInfo> activeHttpRequests_;

    void executeRequestImpl(std::unique_ptr<BaseRequest> request);
    void setErrorCodeAndEmitRequestFinished(BaseRequest *request, ApiRetCode retCode);

    void onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data);
    // This callback function is necessary to cancel the request as quickly as possible if it was canceled on the calling side
    void onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal);

    void clearSessionToken();
    void setSessionTokenExpiry();
};

} // namespace wsnet
