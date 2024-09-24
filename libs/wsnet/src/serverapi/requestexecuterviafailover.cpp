#include "requestexecuterviafailover.h"
#include <spdlog/spdlog.h>
#include "serverapi_utils.h"

namespace wsnet {

RequestExecuterViaFailover::RequestExecuterViaFailover(WSNetHttpNetworkManager *httpNetworkManager, std::unique_ptr<BaseRequest> request, std::unique_ptr<BaseFailover> failover,
                                                       bool bIgnoreSslErrors, bool isConnectedVpnState, WSNetAdvancedParameters *advancedParameters, FailedFailovers &failedFailovers,
                                                       RequestExecuterViaFailoverCallback callback) :
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters),
    request_(std::move(request)),
    failover_(std::move(failover)),
    bIgnoreSslErrors_(bIgnoreSslErrors),
    isConnectedVpnState_(isConnectedVpnState),
    isConnectStateChanged_(false),
    callback_(callback),
    failedFailovers_(failedFailovers)
{
}

RequestExecuterViaFailover::~RequestExecuterViaFailover()
{
    if (asyncCallback_) {
        asyncCallback_->cancel();
    }
}

void RequestExecuterViaFailover::start()
{
    // if true then a result is ready immediately
    // otherwise we are waiting for the onFailoverCallback
    if (failover_->getData(bIgnoreSslErrors_, failoverData_, std::bind(&RequestExecuterViaFailover::onFailoverCallback, this, std::placeholders::_1))) {
        onFailoverCallback(failoverData_);
    }
}

void RequestExecuterViaFailover::setIsConnectedToVpnState(bool isConnected)
{
    if (isConnectedVpnState_ != isConnected) {
        isConnectStateChanged_ = true;
    }
}

void RequestExecuterViaFailover::onFailoverCallback(const std::vector<FailoverData> &data)
{
    // if connect state changed then we can't be sure what failover worked right. Must repeat the request in ServerAPI
    if (isConnectStateChanged_) {
        callback_(RequestExecuterRetCode::kConnectStateChanged, std::move(request_), FailoverData(""));
        return;
    }
    if (data.empty()) {
        callback_(RequestExecuterRetCode::kFailoverFailed, std::move(request_), FailoverData(""));
        return;
    }

    if (request_->isCanceled()) {
        callback_(RequestExecuterRetCode::kRequestCanceled, std::move(request_), FailoverData(""));
        return;
    }

    failoverData_ = data;
    curIndFailoverData_ = 0;

    // if we have already tried this domain and it is failed skip it
    // keep in mind the failover can contain several domains
    while (curIndFailoverData_ < failoverData_.size() && failedFailovers_.isContains(failoverData_[curIndFailoverData_]))  {
        spdlog::info("Got an already failed domain, skip it");
        curIndFailoverData_++;
    }
    if (curIndFailoverData_ >= failoverData_.size())
        callback_(RequestExecuterRetCode::kFailoverFailed, std::move(request_), FailoverData(""));
    else
        executeBaseRequest(failoverData_[curIndFailoverData_]);
}

void RequestExecuterViaFailover::executeBaseRequest(const FailoverData &failoverData)
{
    using namespace std::placeholders;
    auto httpRequest = serverapi_utils::createHttpRequestWithFailoverParameters(httpNetworkManager_, failoverData, request_.get(), bIgnoreSslErrors_, advancedParameters_->isAPIExtraTLSPadding());
    httpRequest->setIsDebugLogCurlError(true);
    asyncCallback_ = httpNetworkManager_->executeRequestEx(httpRequest, 0, std::bind(&RequestExecuterViaFailover::onHttpNetworkRequestFinished, this, _1, _2, _3, _4, _5),
                                                           std::bind(&RequestExecuterViaFailover::onHttpNetworkRequestProgressCallback, this, _1, _2, _3));
}

void RequestExecuterViaFailover::onHttpNetworkRequestFinished(std::uint64_t httpRequestId, std::uint32_t elapsedMs, NetworkError errCode, const std::string &curlError, const std::string &data)
{
    asyncCallback_.reset();
    if (request_->isCanceled()) {
        callback_(RequestExecuterRetCode::kRequestCanceled, std::move(request_), FailoverData(""));
        return;
    }

    // if connect state changed then we can't be sure what failover worked right. Must repeat the request in ServerAPI
    if (isConnectStateChanged_) {
        callback_(RequestExecuterRetCode::kConnectStateChanged, std::move(request_), FailoverData(""));
        return;
    }

    if (errCode == NetworkError::kSuccess) {
        request_->handle(data);
        if (advancedParameters_->isLogApiResponce()) {
            spdlog::info("API request {} finished", request_->name());
            spdlog::info("{}", data);
        }
    }

    if (errCode != NetworkError::kSuccess || request_->retCode() == ServerApiRetCode::kIncorrectJson) {
        failedFailovers_.add(failoverData_[curIndFailoverData_]);
        // failover can contain several domains, let's try another one if there is one
        curIndFailoverData_++;
        if (curIndFailoverData_ >= failoverData_.size())
            callback_(RequestExecuterRetCode::kFailoverFailed, std::move(request_), FailoverData(""));
        else
            executeBaseRequest(failoverData_[curIndFailoverData_]);
        return;
    }

    callback_(RequestExecuterRetCode::kSuccess, std::move(request_), failoverData_[curIndFailoverData_]);
}

void RequestExecuterViaFailover::onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    if (request_->isCanceled()) {
        assert(asyncCallback_);
        asyncCallback_->cancel();
        asyncCallback_.reset();
        callback_(RequestExecuterRetCode::kRequestCanceled, std::move(request_), FailoverData(""));
    }
}

} // namespace wsnet
