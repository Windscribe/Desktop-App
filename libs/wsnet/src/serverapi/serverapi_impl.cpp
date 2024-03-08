#include "serverapi_impl.h"
#include <spdlog/spdlog.h>
#include "settings.h"
#include "serverapi_utils.h"

namespace wsnet {

ServerAPI_impl::ServerAPI_impl(WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer, ServerAPISettings &settings,
                               WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters),
    connectState_(connectState),
    failoverContainer_(failoverContainer),
    settings_(settings)
{
    // try reading a failover from the settings
    auto failover = failoverContainer_->failoverById(settings_.failoverId(), &curFailoverInd_);

    // if it fails, use the first one
    if (!failover) {
        spdlog::info("ServerAPI_impl::ServerAPI_impl, use the first failover");
        resetFailover();
    } else {
        curFailoverUid_ = failover->uniqueId();
        failoverState_ = FailoverState::kFromSettingsUnknown;
        spdlog::info("ServerAPI_impl::ServerAPI_impl, use a failover from settings");
    }
}

ServerAPI_impl::~ServerAPI_impl()
{
    for (auto &it : activeHttpRequests_) {
        it.second.asyncCallback_->cancel();
    }
}

void ServerAPI_impl::setApiResolutionsSettings(bool isAutomatic, std::string manualAddress)
{
    if (!isAutomatic && !manualAddress.empty()) {
        if (!utils::isIpAddress(manualAddress)) {
            spdlog::info("ServerAPI_impl::setApiResolutionsSettings, manualAddress = {} is not correct IP, reset to an automatic resolution", manualAddress);
            return;
        }
    }
    apiResolutionSettings_ = ApiResolutionSettings {isAutomatic, manualAddress};
    spdlog::info("ServerAPI_impl::setApiResolutionsSettings, isAutomatic = {}, manualAddress = {}", isAutomatic, manualAddress);
}

void ServerAPI_impl::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
    spdlog::info("ServerAPI_impl::setIgnoreSslErrors, {}", bIgnore);
}

void ServerAPI_impl::resetFailover()
{
    spdlog::info("ServerAPI_impl::resetFailover");
    auto failover = failoverContainer_->first();
    curFailoverUid_ = failover->uniqueId();
    curFailoverInd_ = 0;
    failoverState_ = FailoverState::kUnknown;
    settings_.setFailovedId("");    // clear setting
}

void ServerAPI_impl::setIsConnectedToVpnState(bool isConnected)
{
    // If we use the hostname from the settings then reset it after the first disconnect signal after starting the program
    if (failoverState_ == FailoverState::kFromSettingsReady) {
        if (isConnected) {
            bWasConnectedToVpn_ = true;
        } else if (!isConnected && bWasConnectedToVpn_) {
            resetFailover();
        }
    }

    isConnectedToVpn_ = isConnected;
    if (requestExecutorViaFailover_)
        requestExecutorViaFailover_->setIsConnectedToVpnState(isConnected);
}

void ServerAPI_impl::setTryingBackupEndpointCallback(std::shared_ptr<CancelableCallback<WSNetTryingBackupEndpointCallback> > tryingBackupEndpointCallback)
{
    tryingBackupEndpointCallback_ = tryingBackupEndpointCallback;
}

// execute request if the failover detected or queue
void ServerAPI_impl::executeRequest(std::unique_ptr<BaseRequest> request)
{
    //if request already canceled do nothing
    if (request->isCanceled()) {
        return;
    }

    // check if we are online
    if (!connectState_.isOnline()) {
        request->setRetCode(ServerApiRetCode::kNoNetworkConnection);
        request->callCallback();
        executeWaitingInQueueRequests();
        return;
    }

    // if API resolution settings settled then use IP from the user settings
    if (!apiResolutionSettings_.isAutomatic && !apiResolutionSettings_.manualAddress.empty()) {
        executeRequestImpl(std::move(request), FailoverData(apiResolutionSettings_.manualAddress));
        executeWaitingInQueueRequests();
        return;
    }

    // if failover already in progress then move the request to queue
    if (requestExecutorViaFailover_) {
        // take into account priority
        // in particular, wgConfigsInit, wgConfigsConnect and pingTest should have a higher priority in the queue to avoid potential connection delays
        if (request->priority() == RequestPriority::kHigh)
            queueRequests_.push_front(std::move(request));
        else
            queueRequests_.push_back(std::move(request));
        return;
    }

    if (isConnectedToVpn_) {
        // in the connected mode always use the primary domain
        executeRequestImpl(std::move(request), FailoverData(hostnameForConnectedState()));
        executeWaitingInQueueRequests();
    } else {
        assert(requestExecutorViaFailover_ == nullptr);

        bool bUseFailover = false;
        if (failoverState_ == FailoverState::kFromSettingsUnknown || failoverState_ == FailoverState::kUnknown) {
            bUseFailover = true;
        } else if (failoverState_ == FailoverState::kReady || failoverState_ == FailoverState::kFromSettingsReady) {
            if (!failoverData_->echConfig().empty() && failoverData_->isExpired()) {
                if (failoverState_ == FailoverState::kReady) failoverState_ = FailoverState::kUnknown;
                else if (failoverState_ == FailoverState::kFromSettingsReady) failoverState_ = FailoverState::kFromSettingsUnknown;
                else assert(false);
                bUseFailover = true;
            }
        }

        if (bUseFailover) {
            auto curFailover = failoverContainer_->failoverById(curFailoverUid_);
            spdlog::info("Trying: {}", curFailover->name());

            // Do not emit this signal for the first failover and for the failover from settings
            if (failoverState_ == FailoverState::kUnknown && curFailoverInd_ > 0 && tryingBackupEndpointCallback_)
                tryingBackupEndpointCallback_->call(curFailoverInd_, failoverContainer_->count() - 1);

            // start RequestExecuterViaFailover and wait for the result in the callback function
            using namespace std::placeholders;
            requestExecutorViaFailover_.reset(new RequestExecuterViaFailover(httpNetworkManager_, std::move(request), std::move(curFailover),
                                                                             bIgnoreSslErrors_, isConnectedToVpn_, advancedParameters_,
                                                                             std::bind(&ServerAPI_impl::onRequestExecuterViaFailoverFinished, this, _1, _2, _3)));
            requestExecutorViaFailover_->start();
        } else {
            if (failoverState_ == FailoverState::kReady || failoverState_ == FailoverState::kFromSettingsReady) {
                assert(failoverData_.has_value());
                executeRequestImpl(std::move(request), *failoverData_);
            } else if (failoverState_ == FailoverState::kFailed) {
                logAllFailoversFailed(request.get());
                request->setRetCode(ServerApiRetCode::kFailoverFailed);
                request->callCallback();
            }
        }
    }
}

void ServerAPI_impl::executeRequestImpl(std::unique_ptr<BaseRequest> request, const FailoverData &failoverData)
{
    using namespace std::placeholders;
    auto httpRequest = serverapi_utils::createHttpRequestWithFailoverParameters(httpNetworkManager_, failoverData, request.get(), bIgnoreSslErrors_, advancedParameters_->isAPIExtraTLSPadding());
    std::uint64_t requestId = curUniqueId_++;
    auto asyncCallback_ = httpNetworkManager_->executeRequestEx(httpRequest, requestId, std::bind(&ServerAPI_impl::onHttpNetworkRequestFinished, this, _1, _2, _3, _4),
                                                           std::bind(&ServerAPI_impl::onHttpNetworkRequestProgressCallback, this, _1, _2, _3));
    HttpRequestInfo hti { std::move(request), asyncCallback_};
    activeHttpRequests_[requestId] = std::move(hti);
}

void ServerAPI_impl::executeWaitingInQueueRequests()
{
    // We need a copy here because executeRequest can add requests to queueRequests_ to avoid an infinite loop
    std::deque<std::unique_ptr<BaseRequest>> copyQueueRequests = std::move(queueRequests_);
    queueRequests_.clear();
    while (!copyQueueRequests.empty()) {
        std::unique_ptr<BaseRequest> req = std::move(copyQueueRequests.front());
        copyQueueRequests.pop_front();
        executeRequest(std::move(req));
    }
}

std::string ServerAPI_impl::hostnameForConnectedState() const
{
    return Settings::instance().primaryServerDomain();
}

void ServerAPI_impl::setErrorCodeAndEmitRequestFinished(BaseRequest *request, ServerApiRetCode retCode)
{
    request->setRetCode(retCode);
    request->callCallback();
}

void ServerAPI_impl::onRequestExecuterViaFailoverFinished(RequestExecuterRetCode retCode, std::unique_ptr<BaseRequest> request, FailoverData failoverData)
{
    assert(failoverState_ == FailoverState::kUnknown || failoverState_ == FailoverState::kFromSettingsUnknown);

    std::unique_ptr<RequestExecuterViaFailover> requestExecutorViaFailoverCopy = std::move(requestExecutorViaFailover_);
    requestExecutorViaFailover_.reset();

    if (retCode == RequestExecuterRetCode::kSuccess) {
        if (failoverState_ == FailoverState::kFromSettingsUnknown) {
            failoverState_ = FailoverState::kFromSettingsReady;
        } else {
            failoverState_ = FailoverState::kReady;
            settings_.setFailovedId(curFailoverUid_);
        }
        failoverData_ = failoverData;
        request->callCallback();
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kRequestCanceled) {
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kFailoverFailed) {
        if (failoverState_ == FailoverState::kFromSettingsUnknown) {
            resetFailover();
            executeRequest(std::move(request));
        } else {
            assert(failoverState_ == FailoverState::kUnknown);

            auto nextFailover = failoverContainer_->next(curFailoverUid_);
            if (nextFailover) {
                curFailoverUid_ = nextFailover->uniqueId();
                curFailoverInd_++;
                executeRequest(std::move(request));
            } else {
                failoverState_ = FailoverState::kFailed;

                if (!isFailoverFailedLogAlreadyDone_) {
                    spdlog::info("API request {} failed: API not ready", request->name());
                    isFailoverFailedLogAlreadyDone_ = true;
                }

                logAllFailoversFailed(request.get());
                setErrorCodeAndEmitRequestFinished(request.get(), ServerApiRetCode::kFailoverFailed);
                executeWaitingInQueueRequests();
            }
        }
    } else if (retCode == RequestExecuterRetCode::kConnectStateChanged) {
        // Repeat the execution of the request via failover
        executeRequest(std::move(request));
    } else {
        assert(false);
    }
}

void ServerAPI_impl::onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, NetworkError errCode, const std::string &data)
{
    auto it = activeHttpRequests_.find(requestId);
    assert(it != activeHttpRequests_.end());

    if (it->second.request->isCanceled()) {
        activeHttpRequests_.erase(it);
        return;
    }

    if (errCode == NetworkError::kSuccess) {
        if (advancedParameters_->isLogApiResponce()) {
            spdlog::info("API request {} finished", it->second.request->name());
            spdlog::info("{}", data);
        }
        it->second.request->handle(data);
        it->second.request->callCallback();
    } else {
        setErrorCodeAndEmitRequestFinished(it->second.request.get(), ServerApiRetCode::kNetworkError);
    }
    activeHttpRequests_.erase(it);
}

void ServerAPI_impl::onHttpNetworkRequestProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    auto it = activeHttpRequests_.find(requestId);
    assert(it != activeHttpRequests_.end());
    if (it->second.request->isCanceled()) {
        it->second.asyncCallback_->cancel();
        activeHttpRequests_.erase(it);
    }
}

void ServerAPI_impl::logAllFailoversFailed(BaseRequest *request)
{
    if (!isFailoverFailedLogAlreadyDone_) {
        spdlog::info("API request {} failed: API not ready", request->name());
        isFailoverFailedLogAlreadyDone_ = true;
    }
}

} // namespace wsnet
