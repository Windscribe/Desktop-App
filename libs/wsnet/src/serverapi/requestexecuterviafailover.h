#pragma once

#include "WSNetHttpNetworkManager.h"
#include "WSNetAdvancedParameters.h"
#include <mutex>
#include <thread>
#include "baserequest.h"
#include "failover/basefailover.h"

namespace wsnet {

// Helper class used by ServerAPI.
// Tries to execute a request through the specified failover and returns the result of this execution.
// In short it executes the failover request first and then, if successful, the request itself

enum class RequestExecuterRetCode { kSuccess, kRequestCanceled, kFailoverFailed, kConnectStateChanged};

typedef std::function<void(RequestExecuterRetCode retCode, std::unique_ptr<BaseRequest> request, FailoverData failoverData)> RequestExecuterViaFailoverCallback;

// Not thread safe
class RequestExecuterViaFailover
{
public:
    // The request starts executing from the constructor immediately
    explicit RequestExecuterViaFailover(WSNetHttpNetworkManager *httpNetworkManager, std::unique_ptr<BaseRequest> request, std::unique_ptr<BaseFailover> failover,
                                        bool bIgnoreSslErrors, bool isConnectedVpnState, WSNetAdvancedParameters *advancedParameters, RequestExecuterViaFailoverCallback callback);
    virtual ~RequestExecuterViaFailover();

    void start();
    void setIsConnectedToVpnState(bool isConnected);

private:
    WSNetHttpNetworkManager *httpNetworkManager_;
    WSNetAdvancedParameters *advancedParameters_;
    RequestExecuterViaFailoverCallback callback_;

    std::unique_ptr<BaseRequest> request_;
    std::unique_ptr<BaseFailover> failover_;
    bool bIgnoreSslErrors_;
    bool isConnectedVpnState_;
    bool isConnectStateChanged_;

    std::shared_ptr<WSNetCancelableCallback> asyncCallback_;

    std::vector<FailoverData> failoverData_;
    int curIndFailoverData_;

    void onFailoverCallback(const std::vector<FailoverData> &data);
    void executeBaseRequest(const FailoverData &failoverData);
    void onHttpNetworkRequestFinished(std::uint64_t httpRequestId, std::uint32_t elapsedMs, NetworkError errCode, const std::string &data);
    // This callback function is necessary to cancel the request as quickly as possible if it was canceled on the calling side
    void onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal);
};

} // namespace wsnet
