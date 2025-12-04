#pragma once

#include <skyr/url.hpp>
#include "WSNetHttpNetworkManager.h"
#include "../baserequest.h"

namespace wsnet {

// This request is different from all the others, so it's placed in a separate class.
// This request is always made through the primary domain windscribe.com and also has a backup domain in case the first one returns something unexpected.
// This request is intended for connectivity tests only and it does not use a failover mechanism.
class PingTest
{
public:
    explicit PingTest(WSNetHttpNetworkManager *httpNetworkManager);
    virtual ~PingTest() {};

    void doPingTest(std::uint32_t timeoutMs, RequestFinishedCallback callback);


private:
    WSNetHttpNetworkManager *httpNetworkManager_;
    std::uint64_t curUniqueId_ = 0;
    std::vector<skyr::url> endpoints_;

    struct RequestInfo {
        RequestFinishedCallback callback;
        int curEndpointInd;
        std::uint32_t timeoutMs;
    };
    std::map<std::uint64_t, RequestInfo> activeRequests_;

    void onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data);
};

} // namespace wsnet

