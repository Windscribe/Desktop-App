#include "pingtest.h"
#include <skyr/url.hpp>
#include "utils/wsnet_logger.h"
#include "utils/urlquery_utils.h"
#include "utils/utils.h"

namespace wsnet {

struct PingTest::EndpointsImpl {
    std::vector<skyr::url> endpoints;
};

PingTest::~PingTest() = default;

PingTest::PingTest(WSNetHttpNetworkManager *httpNetworkManager) : httpNetworkManager_(httpNetworkManager),
    endpointsImpl_(std::make_unique<EndpointsImpl>())
{
    auto tunnelTestEndpoints = Settings::instance().tunnelTestEndpoints();
    for (const std::string &it : tunnelTestEndpoints) {
        skyr::url url("https://" + it);
        auto &sp = url.search_parameters();
        urlquery_utils::addPlatformQueryItems(sp);
        endpointsImpl_->endpoints.push_back(url);
    }
}

void PingTest::doPingTest(std::uint32_t timeoutMs, RequestFinishedCallback callback)
{
    assert(!endpointsImpl_->endpoints.empty());
    assert(callback != nullptr);
    // Do the first ping test through the primary domain
    auto httpRequest = httpNetworkManager_->createGetRequest(endpointsImpl_->endpoints[0].c_str(), timeoutMs, false);
    // We do not use DNS cache, as we need to verify the functionality of the connected VPN's DNS.
    httpRequest->setUseDnsCache(false);
    httpRequest->setIsWhiteListIps(false);

    using namespace std::placeholders;
    std::uint64_t requestId = curUniqueId_++;
    RequestInfo ri { callback, 0, timeoutMs};
    activeRequests_[requestId] = ri;
    httpNetworkManager_->executeRequestEx(httpRequest, requestId, std::bind(&PingTest::onHttpNetworkRequestFinished, this, _1, _2, _3, _4));
}

void PingTest::onHttpNetworkRequestFinished(std::uint64_t requestId, std::uint32_t elapsedMs, std::shared_ptr<WSNetRequestError> error, const std::string &data)
{
    auto it = activeRequests_.find(requestId);
    assert(it != activeRequests_.end());

    if (it->second.callback->isCanceled()) {
        activeRequests_.erase(it);
        return;
    }

    bool bSuccess = false;
    if (error->isSuccess()) {
        auto trimmedData = utils::trim(data);
        // Extract HTTP response code from the error object. It must be 200 and returned data must be a valid IP address.
        if (error->httpResponseCode() == 200 && utils::isIpAddress(trimmedData)) {
            it->second.callback->call(ApiRetCode::kSuccess, trimmedData);
            bSuccess = true;
        } else if (it->second.curEndpointInd < (endpointsImpl_->endpoints.size() - 1)) {
            // try next backup endpoint
            it->second.curEndpointInd++;
            g_logger->info("Trying backup endpoint for PingTest: {}", endpointsImpl_->endpoints[it->second.curEndpointInd].domain()->c_str());
            auto httpRequest = httpNetworkManager_->createGetRequest(endpointsImpl_->endpoints[it->second.curEndpointInd].c_str(), it->second.timeoutMs, false);
            httpRequest->setUseDnsCache(false);
            httpRequest->setIsWhiteListIps(false);
            using namespace std::placeholders;
            httpNetworkManager_->executeRequestEx(httpRequest, requestId, std::bind(&PingTest::onHttpNetworkRequestFinished, this, _1, _2, _3, _4));
            return;
        }
    }
    if (!bSuccess) {
        it->second.callback->call(ApiRetCode::kNetworkError, "");
    }
    activeRequests_.erase(it);
}


} // namespace wsnet

