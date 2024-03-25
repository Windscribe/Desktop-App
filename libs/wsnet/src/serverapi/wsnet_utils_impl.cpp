#include "wsnet_utils_impl.h"
#include "requestsfactory.h"

namespace wsnet {

WSNetUtils_impl::WSNetUtils_impl(BS::thread_pool &taskQueue, WSNetHttpNetworkManager *httpNetworkManager,
                                 IFailoverContainer *failoverContainer, WSNetAdvancedParameters *advancedParameters) :
    taskQueue_(taskQueue),
    httpNetworkManager_(httpNetworkManager),
    advancedParameters_(advancedParameters),
    failoverContainer_(failoverContainer)
{
}

WSNetUtils_impl::~WSNetUtils_impl()
{
    activeRequests_.clear();
}

std::int32_t WSNetUtils_impl::failoverCount() const
{
    return failoverContainer_->count();
}

std::string WSNetUtils_impl::failoverName(int failoverInd) const
{
    return failoverByInd(failoverInd)->name();
}

std::shared_ptr<WSNetCancelableCallback> WSNetUtils_impl::myIPViaFailover(int failoverInd, WSNetRequestFinishedCallback callback)
{
    auto cancelableCallback = std::make_shared<CancelableCallback<WSNetRequestFinishedCallback>>(callback);
    BaseRequest *request = requests_factory::myIP(cancelableCallback);
    taskQueue_.detach_task([this, failoverInd, request] { myIPViaFailover_impl(failoverInd, std::unique_ptr<BaseRequest>(request)); });
    return cancelableCallback;
}

void WSNetUtils_impl::myIPViaFailover_impl(int failoverInd, std::unique_ptr<BaseRequest> request)
{
    using namespace std::placeholders;

    auto failover = failoverByInd(failoverInd);
    RequestExecuterViaFailover *requestExecutorViaFailover = new RequestExecuterViaFailover(httpNetworkManager_, std::move(request), std::move(failover),
                                                                                            false, false, advancedParameters_, failedFailovers_,
                                                                       std::bind(&WSNetUtils_impl::onRequestExecuterViaFailoverFinished, this, _1, _2, _3, curUniqueId_));
    activeRequests_[curUniqueId_] = std::unique_ptr<RequestExecuterViaFailover>(requestExecutorViaFailover);
    curUniqueId_++;
    requestExecutorViaFailover->start();
}

void WSNetUtils_impl::onRequestExecuterViaFailoverFinished(RequestExecuterRetCode retCode, std::unique_ptr<BaseRequest> request, FailoverData failoverData, std::uint64_t id)
{
    activeRequests_.erase(id);

    if (retCode == RequestExecuterRetCode::kSuccess) {
        request->callCallback();
    } else if (retCode == RequestExecuterRetCode::kRequestCanceled) {
        // nothing todo
    } else if (retCode == RequestExecuterRetCode::kFailoverFailed) {
        request->setRetCode(ServerApiRetCode::kFailoverFailed);
        request->callCallback();
    } else if (retCode == RequestExecuterRetCode::kConnectStateChanged) {
        request->setRetCode(ServerApiRetCode::kNetworkError);
        request->callCallback();
    } else {
        assert(false);
    }
}

std::unique_ptr<BaseFailover> WSNetUtils_impl::failoverByInd(int ind) const
{
    assert (ind >= 0 && ind < failoverContainer_->count());
    int i = 0;
    std::unique_ptr<BaseFailover> failover = failoverContainer_->first();
    assert(failover);

    while (failover) {
        if (ind == i)
            return failover;
        failover = failoverContainer_->next(failover->uniqueId());
        i++;
    }
    return nullptr;
}


} // namespace wsnet
