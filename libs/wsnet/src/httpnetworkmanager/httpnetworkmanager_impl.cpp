#include "httpnetworkmanager_impl.h"
#include <spdlog/spdlog.h>
#include "utils/utils.h"

namespace wsnet {

HttpNetworkManager_impl::HttpNetworkManager_impl(boost::asio::io_context &io_context, WSNetDnsResolver *dnsResolver) :
    io_context_(io_context),
    dnsCache_(dnsResolver, std::bind(&HttpNetworkManager_impl::onDnsResolvedCallback, this, std::placeholders::_1)),
    curlNetworkManager_(std::bind(&HttpNetworkManager_impl::onCurlFinishedCallback, this, std::placeholders::_1, std::placeholders::_2),
                        std::bind(&HttpNetworkManager_impl::onCurlProgressCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                        std::bind(&HttpNetworkManager_impl::onCurlReadyDataCallback, this, std::placeholders::_1, std::placeholders::_2))
{
}

HttpNetworkManager_impl::~HttpNetworkManager_impl()
{
}

bool HttpNetworkManager_impl::init()
{
    return curlNetworkManager_.init();
}

void HttpNetworkManager_impl::executeRequest(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t id, std::shared_ptr<HttpNetworkManagerCallbacks> callbacks)
{
    if (callbacks->isCanceled())
        return;

    requestsMap_[curRequestId_] = RequestData { request, id, callbacks, std::chrono::steady_clock::now() };

    // skip DNS-resolution if the overrideIp is settled
    if (!request->overrideIp().empty()) {
        std::vector<std::string> ips;
        ips.push_back(request->overrideIp());
        onDnsResolvedImpl( DnsCacheResult { curRequestId_, true, ips, false });
    } else {
        DnsCacheResult result;
        // take into account that a SNI domain can be specified
        if (!request->sniDomain().empty()) {
            result = dnsCache_.resolve(curRequestId_, request->sniDomain(), !request->isUseDnsCache());
        } else {
            result = dnsCache_.resolve(curRequestId_, request->hostname(), !request->isUseDnsCache());
        }
        if (result.bFromCache) {
            onDnsResolvedImpl(result);
        }
    }

    curRequestId_++;
}

void HttpNetworkManager_impl::setProxySettings(const std::string &address, const std::string &username, const std::string &password)
{
    curlNetworkManager_.setProxySettings(address, username, password);
}

void HttpNetworkManager_impl::setWhitelistIpsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistIpsCallback>> callback)
{
    whitelistIpsCallback_ = callback;
    isWhitelistCallbackChanged_ = true;
}

void HttpNetworkManager_impl::setWhitelistSocketsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback> > callback)
{
    curlNetworkManager_.setWhitelistSocketsCallback(callback);
}

void HttpNetworkManager_impl::onDnsResolvedCallback(const DnsCacheResult &result)
{
    boost::asio::post(io_context_, [this, result] {
        onDnsResolvedImpl(result);
    });
}

void HttpNetworkManager_impl::onDnsResolvedImpl(const DnsCacheResult &result)
{
    auto request = requestsMap_.find(result.id);
    assert(request != requestsMap_.end());

    if (request->second.callbacks->isCanceled()) { // the request was canceled
        requestsMap_.erase(request);
        return;
    }

    if (!result.bSuccess) {
        request->second.callbacks->callFinished(request->second.userDataId, (std::uint32_t)utils::since(request->second.startTime).count(), NetworkError::kDnsResolveError, std::string());
        requestsMap_.erase(request);
        return;
    }

    if (request->second.request->timeoutMs() <= result.elapsedMs) {
        request->second.callbacks->callFinished(request->second.userDataId, (std::uint32_t)utils::since(request->second.startTime).count(), NetworkError::kTimeoutExceed, std::string());
        requestsMap_.erase(request);
        return;
    }

    request->second.ips = result.ips;

    if (request->second.request->isWhiteListIps())
        whitelistIps(result.ips);

    curlNetworkManager_.executeRequest(request->first, request->second.request, result.ips, request->second.request->timeoutMs() - result.elapsedMs);
}

void HttpNetworkManager_impl::onCurlFinishedCallback(std::uint64_t requestId, bool bSuccess)
{
    boost::asio::post(io_context_, [this, requestId, bSuccess] {
        onCurlFinishedCallbackImpl(requestId, bSuccess);
    });
}

void HttpNetworkManager_impl::onCurlProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    boost::asio::post(io_context_, [this, requestId, bytesReceived, bytesTotal] {
        onCurlProgressCallbackImpl(requestId, bytesReceived, bytesTotal);
    });
}

void HttpNetworkManager_impl::onCurlReadyDataCallback(std::uint64_t requestId, const std::string &data)
{
    boost::asio::post(io_context_, [this, requestId, data] {
        onCurlReadyDataCallbackImpl(requestId, data);
    });
}

void HttpNetworkManager_impl::onCurlFinishedCallbackImpl(std::uint64_t requestId, bool bSuccess)
{
    auto request = requestsMap_.find(requestId);
    if (request != requestsMap_.end()) {
        NetworkError networkError = (bSuccess ? NetworkError::kSuccess : NetworkError::kCurlError);
        RequestData &rd = request->second;
        rd.callbacks->callFinished(rd.userDataId, utils::since(rd.startTime).count(), networkError, rd.data);
        if (rd.request->isRemoveFromWhitelistIpsAfterFinish())
            removeWhitelistIps(rd.ips);
        requestsMap_.erase(requestId);
    }
}

void HttpNetworkManager_impl::onCurlProgressCallbackImpl(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal)
{
    auto request = requestsMap_.find(requestId);
    if (request != requestsMap_.end()) {
        RequestData &rd = request->second;
        if (rd.callbacks->isCanceled()) {
            cancelAndRemoveRequest(request);
        } else {
            rd.callbacks->callProgress(rd.userDataId, bytesReceived, bytesTotal);
        }
    }
}

void HttpNetworkManager_impl::onCurlReadyDataCallbackImpl(std::uint64_t requestId, const std::string &data)
{
    auto request = requestsMap_.find(requestId);
    if (request != requestsMap_.end()) {
        RequestData &rd = request->second;
        if (rd.callbacks->isCanceled()) {
            cancelAndRemoveRequest(request);
        } else {
            if (!rd.callbacks->isDataReadyNull()) {
                // call callback
                rd.callbacks->callDataReady(rd.userDataId, data);
            } else {
                // append data to internal data buffer
                request->second.data.reserve( request->second.data.size() + data.size() );
                request->second.data.insert( request->second.data.end(), data.begin(), data.end() );
            }
        }
    }
}

void HttpNetworkManager_impl::whitelistIps(const std::vector<std::string> &ips)
{
    bool bChanged = isWhitelistCallbackChanged_;
    for (const auto &ip : ips) {
        auto res = whitelistIps_.insert(ip);
        if (res.second)
            bChanged = true;
    }
    if (bChanged && whitelistIpsCallback_) {
        whitelistIpsCallback_->call(whitelistIps_);
        isWhitelistCallbackChanged_ = false;
    }
}

void HttpNetworkManager_impl::removeWhitelistIps(const std::vector<std::string> &ips)
{
    bool bChanged = isWhitelistCallbackChanged_;
    for (const auto &ip : ips) {
        if (whitelistIps_.erase(ip))
            bChanged = true;
    }
    if (bChanged && whitelistIpsCallback_) {
        whitelistIpsCallback_->call(whitelistIps_);
        isWhitelistCallbackChanged_ = false;
    }
}

void HttpNetworkManager_impl::cancelAndRemoveRequest(const std::map<std::uint64_t, RequestData>::iterator &request)
{
    RequestData &rd = request->second;
    curlNetworkManager_.cancelRequest(request->first);
    if (rd.request->isRemoveFromWhitelistIpsAfterFinish())
        removeWhitelistIps(rd.ips);
    requestsMap_.erase(request);
}

} // namespace wsnet

