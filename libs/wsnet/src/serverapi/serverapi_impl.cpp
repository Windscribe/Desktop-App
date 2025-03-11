#include "serverapi_impl.h"
#include "utils/wsnet_logger.h"
#include "settings.h"
#include "serverapi_utils.h"

namespace wsnet {

ServerAPI_impl::ServerAPI_impl(WSNetHttpNetworkManager *httpNetworkManager, IFailoverContainer *failoverContainer, PersistentSettings &persistentSettings,
                               WSNetAdvancedParameters *advancedParameters, ConnectState &connectState) :
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters),
    connectState_(connectState),
    failoverContainer_(failoverContainer),
    persistentSettings_(persistentSettings),
    curInternalFailoverInd_(0),
    failoverState_(FailoverState::kUnknown)
{
    // try reading a failover from the settings
    auto failover = failoverContainer_->failoverById(persistentSettings_.failoverId());

    // if it fails, use the first one
    if (!failover) {
        g_logger->info("ServerAPI_impl::ServerAPI_impl, use the first failover");
        resetFailover();
    } else {
        curFailoverUid_ = failover->uniqueId();
        startFailoverUid_ = curFailoverUid_;
        g_logger->info("ServerAPI_impl::ServerAPI_impl, use a failover from settings");
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
            g_logger->error("ServerAPI_impl::setApiResolutionsSettings, manualAddress = {} is not correct IP, reset to an automatic resolution", manualAddress);
            return;
        }
    }
    apiResolutionSettings_ = ApiResolutionSettings {isAutomatic, manualAddress};
    g_logger->info("ServerAPI_impl::setApiResolutionsSettings, isAutomatic = {}, manualAddress = {}", isAutomatic, manualAddress);
}

void ServerAPI_impl::setIgnoreSslErrors(bool bIgnore)
{
    if (!bIgnoreSslErrors_ && bIgnore && failoverState_ != FailoverState::kReady) {
        // User turned on ignore SSL errors.  If the API is currently not ready, reset failover
        resetFailover();
    }
    bIgnoreSslErrors_ = bIgnore;
    g_logger->info("ServerAPI_impl::setIgnoreSslErrors, {}", bIgnore);
}

void ServerAPI_impl::resetFailover()
{
    g_logger->info("ServerAPI_impl::resetFailover");
    resetFailoverImpl(true);
}

void ServerAPI_impl::setIsConnectedToVpnState(bool isConnected)
{
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
        if (failoverState_ == FailoverState::kUnknown) {
            bUseFailover = true;
        } else if (failoverState_ == FailoverState::kReady) {
            if (failoverData_->isExpired()) {
                g_logger->info("The current failover domain is expired. Reset the failover state.");
                failoverState_ = FailoverState::kUnknown;
                bUseFailover = true;
            }
        }

        if (bUseFailover) {
            auto curFailover = failoverContainer_->failoverById(curFailoverUid_);
            g_logger->info("Trying: {}", curFailover->name());

            // Do not emit this signal for the first failover
            if (failoverState_ == FailoverState::kUnknown && curInternalFailoverInd_ > 0 && tryingBackupEndpointCallback_)
                tryingBackupEndpointCallback_->call(curInternalFailoverInd_, failoverContainer_->count() - 1);

            // start RequestExecuterViaFailover and wait for the result in the callback function
            using namespace std::placeholders;
            requestExecutorViaFailover_.reset(new RequestExecuterViaFailover(httpNetworkManager_, std::move(request), std::move(curFailover),
                                                                             bIgnoreSslErrors_, isConnectedToVpn_, advancedParameters_, failedFailovers_,
                                                                             std::bind(&ServerAPI_impl::onRequestExecuterViaFailoverFinished, this, _1, _2, _3)));
            requestExecutorViaFailover_->start();
        } else {
            if (failoverState_ == FailoverState::kReady) {
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
    httpRequest->setIsDebugLogCurlError(true);
    std::uint64_t requestId = curUniqueId_++;
    auto asyncCallback_ = httpNetworkManager_->executeRequestEx(httpRequest, requestId, std::bind(&ServerAPI_impl::onHttpNetworkRequestFinished, this, _1, _2, _3, _4),
                                                           std::bind(&ServerAPI_impl::onHttpNetworkRequestProgressCallback, this, _1, _2, _3));
    HttpRequestInfo hti { std::move(request), asyncCallback_, !isConnectedToVpn_, false};
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
    assert(failoverState_ == FailoverState::kUnknown);

    std::unique_ptr<RequestExecuterViaFailover> requestExecutorViaFailoverCopy = std::move(requestExecutorViaFailover_);
    requestExecutorViaFailover_.reset();

    if (retCode == RequestExecuterRetCode::kSuccess) {
        failoverState_ = FailoverState::kReady;
        persistentSettings_.setFailoverId(curFailoverUid_);
        failoverData_ = failoverData;
        request->callCallback();
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kRequestCanceled) {
        executeWaitingInQueueRequests();
    } else if (retCode == RequestExecuterRetCode::kFailoverFailed) {
        auto nextFailover = getNextFailover();
        if (nextFailover) {
            curFailoverUid_ = nextFailover->uniqueId();
            curInternalFailoverInd_++;
            executeRequest(std::move(request));
        } else {
            // If there was at least one successful request, we never go to state FailoverState::kFailed
            // In this case we go through all the failovers again
            if (bWasSuccesfullRequest_) {
                g_logger->info("API request {} failed: all failovers failed, reset failover", request->name());
                resetFailoverImpl(false);
                setErrorCodeAndEmitRequestFinished(request.get(), ServerApiRetCode::kNetworkError);
                executeWaitingInQueueRequests();
            } else {
                failoverState_ = FailoverState::kFailed;
                if (!isFailoverFailedLogAlreadyDone_) {
                    g_logger->info("API request {} failed: API not ready", request->name());
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
    } else if (retCode == RequestExecuterRetCode::kNoNetwork) {
        setErrorCodeAndEmitRequestFinished(request.get(), ServerApiRetCode::kNoNetworkConnection);
        executeWaitingInQueueRequests();
    } else {
        assert(false);
    }
}

void ServerAPI_impl::onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data)
{
    auto it = activeHttpRequests_.find(requestId);
    assert(it != activeHttpRequests_.end());

    if (it->second.request->isCanceled()) {
        activeHttpRequests_.erase(it);
        return;
    }

    if (error->isSuccess()) {
        if (advancedParameters_->isLogApiResponce()) {
            g_logger->info("API request {} finished", it->second.request->name());
            g_logger->info("{}", data);
        }
        bWasSuccesfullRequest_ = true;
        it->second.request->handle(data);
        it->second.request->callCallback();
    } else if (error->isNoNetworkError()) {
        g_logger->info("API request {} failed with error = {}", it->second.request->name(), error->toString());
        setErrorCodeAndEmitRequestFinished(it->second.request.get(), ServerApiRetCode::kNoNetworkConnection);
    } else {
        g_logger->info("API request {} failed with error = {}", it->second.request->name(), error->toString());

        // we need to start going through the backup domains again
        // except for the DNS resolve error
        if (it->second.bFromDisconnectedVPNState_ && (!error->isDnsError() || it->second.request->retCode() == ServerApiRetCode::kIncorrectJson)) {

            if (!it->second.bDiscard) {
                resetFailoverImpl(false);
                g_logger->info("ServerAPI_impl::onHttpNetworkRequestFinished, reset failover");

                // mark all pending requests for discard
                for (auto &req : activeHttpRequests_) {
                    req.second.bDiscard = true;
                }
            }

            // Repeat the execution of the request via failover
            executeRequest(std::move(it->second.request));

        } else {
            setErrorCodeAndEmitRequestFinished(it->second.request.get(), ServerApiRetCode::kNetworkError);
        }
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

std::unique_ptr<BaseFailover> ServerAPI_impl::getNextFailover()
{
    // switching to the next failover
    // we consider the last one to be the one we started with,  i.e. startFailoverUid_
    auto nextFailover = failoverContainer_->next(curFailoverUid_);
    if (!nextFailover) {
        // looping
        nextFailover = failoverContainer_->first();
    }

    if (nextFailover->uniqueId() == startFailoverUid_)
        return nullptr;
    else
        return nextFailover;
}

void ServerAPI_impl::resetFailoverImpl(bool toFirst)
{
    std::unique_ptr<BaseFailover> failover;
    if (toFirst) {
        failover = failoverContainer_->first();
    } else {
        startFailoverUid_.clear();
        failover = getNextFailover();
        assert(failover);
    }

    curFailoverUid_ = failover->uniqueId();
    startFailoverUid_ = curFailoverUid_;
    curInternalFailoverInd_ = 0;
    failoverState_ = FailoverState::kUnknown;
    failedFailovers_.clear();
    persistentSettings_.setFailoverId("");    // clear setting
}

void ServerAPI_impl::logAllFailoversFailed(BaseRequest *request)
{
    if (!isFailoverFailedLogAlreadyDone_) {
        g_logger->info("API request {} failed: API not ready", request->name());
        isFailoverFailedLogAlreadyDone_ = true;
    }
}

} // namespace wsnet
