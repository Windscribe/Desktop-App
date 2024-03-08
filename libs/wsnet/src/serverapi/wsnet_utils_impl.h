#pragma once

#include "WSNetUtils.h"
#include <BS_thread_pool.hpp>
#include "WSNetHttpNetworkManager.h"
#include "failover/ifailovercontainer.h"
#include "requestexecuterviafailover.h"

namespace wsnet {

class WSNetUtils_impl : public WSNetUtils
{
public:
    explicit WSNetUtils_impl(BS::thread_pool &taskQueue, WSNetHttpNetworkManager *httpNetworkManager,
                             IFailoverContainer *failoverContainer, WSNetAdvancedParameters *advancedParameters);
    virtual ~WSNetUtils_impl();

    std::int32_t failoverCount() const override;
    std::string failoverName(int failoverInd) const override;

    std::shared_ptr<WSNetCancelableCallback> myIPViaFailover(int failoverInd, WSNetRequestFinishedCallback callback) override;

private:
    BS::thread_pool &taskQueue_;
    WSNetHttpNetworkManager *httpNetworkManager_;
    WSNetAdvancedParameters *advancedParameters_;
    IFailoverContainer *failoverContainer_;
    std::uint64_t curUniqueId_ = 0;
    std::map<std::uint64_t, std::unique_ptr<RequestExecuterViaFailover> > activeRequests_;

    void myIPViaFailover_impl(int failoverInd, std::unique_ptr<BaseRequest> request);
    void onRequestExecuterViaFailoverFinished(RequestExecuterRetCode retCode, std::unique_ptr<BaseRequest> request, FailoverData failoverData, std::uint64_t id);
    std::unique_ptr<BaseFailover> failoverByInd(int ind) const;
};

} // namespace wsnet
