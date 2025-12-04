#pragma once

#include "WSNetServerAPI.h"
#include <mutex>
#include <queue>
#include <map>
#include <optional>
#include <atomic>
#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include "../baserequest.h"
#include "connectstate.h"
#include "failover/ifailovercontainer.h"
#include "failover/failoverdata.h"
#include "requestexecuterviafailover.h"
#include "utils/cancelablecallback.h"
#include "utils/persistentsettings.h"
#include "connectstate.h"
#include "failedfailovers.h"

namespace wsnet {

class ServerAPI_impl
{
public:
    explicit ServerAPI_impl(WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer,
                            PersistentSettings &persistentSettings, WSNetAdvancedParameters *advancedParameters, ConnectState &connectState);
    virtual ~ServerAPI_impl();

    void setApiResolutionsSettings(const std::string &apiRoot, const std::string &assetsRoot);
    void setIgnoreSslErrors(bool bIgnore);
    void resetFailover();
    void setIsConnectedToVpnState(bool isConnected);
    void setTryingBackupEndpointCallback(std::shared_ptr<CancelableCallback<WSNetTryingBackupEndpointCallback>> tryingBackupEndpointCallback);

    void executeRequest(std::unique_ptr<BaseRequest> request);

private:
    WSNetHttpNetworkManager *httpNetworkManager_;
    WSNetAdvancedParameters *advancedParameters_;
    ConnectState &connectState_;
    IFailoverContainer *failoverContainer_;
    std::shared_ptr<CancelableCallback<WSNetTryingBackupEndpointCallback>> tryingBackupEndpointCallback_ = nullptr;

    std::deque<std::unique_ptr<BaseRequest>> queueRequests_;    // a queue of requests that are waiting for the failover to complete

    std::uint64_t curUniqueId_ = 0;     // for generate unique identifiers for HTTP-requests

    PersistentSettings &persistentSettings_;    // The ServerAPISettings class is protected by mutex, so it's thread-safe

    ApiOverrideSettings apiOverrideSettings_;

    bool bIgnoreSslErrors_ = false;
    bool isConnectedToVpn_ = false;

    struct HttpRequestInfo {
        std::unique_ptr<BaseRequest> request;
        std::shared_ptr<WSNetCancelableCallback> asyncCallback_;
        bool bFromDisconnectedVPNState_;
        bool bDiscard;
    };
    std::map<std::uint64_t, HttpRequestInfo> activeHttpRequests_;

    // current failover state
    std::string curFailoverUid_;
    std::string startFailoverUid_;
    int curInternalFailoverInd_;
    enum class FailoverState { kUnknown, kReady, kFailed } failoverState_;
    std::unique_ptr<RequestExecuterViaFailover> requestExecutorViaFailover_;
    std::optional<FailoverData> failoverData_;      // valid only in kReady state
    bool isFailoverFailedLogAlreadyDone_ = false;   // log "failover failed: API not ready" only once to avoid spam
    FailedFailovers failedFailovers_;
    bool bWasSuccesfullRequest_ = false;    // was at least one successful request?

    void executeRequest(std::uint64_t requestId);
    void executeRequestImpl(std::unique_ptr<BaseRequest> request, const FailoverData &failoverData);
    void executeWaitingInQueueRequests();
    std::string hostnameForConnectedState() const;
    void setErrorCodeAndEmitRequestFinished(BaseRequest *request, ServerApiRetCode retCode);

    void onRequestExecuterViaFailoverFinished(RequestExecuterRetCode retCode, std::unique_ptr<BaseRequest> request, FailoverData failoverData);

    void onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data);
    // This callback function is necessary to cancel the request as quickly as possible if it was canceled on the calling side
    void onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal);

    std::unique_ptr<BaseFailover> getNextFailover();
    void resetFailoverImpl(bool toFirst);

    void logAllFailoversFailed(BaseRequest *request);
};

} // namespace wsnet
