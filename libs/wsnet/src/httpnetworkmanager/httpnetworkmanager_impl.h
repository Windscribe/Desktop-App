#pragma once

#include "WSNetHttpNetworkManager.h"
#include <mutex>
#include <boost/asio.hpp>
#include "WSNetDnsResolver.h"
#include "curlnetworkmanager.h"
#include "dnscache.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

using HttpNetworkManagerCallbacks = CancelableCallback3<WSNetHttpNetworkManagerFinishedCallback, WSNetHttpNetworkManagerProgressCallback, WSNetHttpNetworkManagerReadyDataCallback>;

class HttpNetworkManager_impl
{
public:
    HttpNetworkManager_impl(boost::asio::io_context &io_context, WSNetDnsResolver *dnsResolver);
    virtual ~HttpNetworkManager_impl();

    bool init();

    void executeRequest(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t id,
                        std::shared_ptr<HttpNetworkManagerCallbacks> callbacks);

    void setProxySettings(const std::string &address,
                          const std::string &username, const std::string &password);

    void setWhitelistIpsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistIpsCallback> > callback);
    void setWhitelistSocketsCallback(std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback> > callback);

    void clearDnsCache();

private:
    boost::asio::io_context &io_context_;
    DnsCache dnsCache_;
    CurlNetworkManager curlNetworkManager_;
    std::shared_ptr<CancelableCallback<WSNetHttpNetworkManagerWhitelistIpsCallback> > whitelistIpsCallback_;
    bool isWhitelistCallbackChanged_ = false;


    struct RequestData
    {
        std::shared_ptr<WSNetHttpRequest> request;
        std::uint64_t userDataId;
        std::shared_ptr<HttpNetworkManagerCallbacks> callbacks;
        std::chrono::steady_clock::time_point startTime;
        std::vector<std::string> ips;
        std::string data;
    };

    std::map<std::uint64_t, RequestData> requestsMap_;
    std::uint64_t curRequestId_ = 0;

    std::set<std::string> whitelistIps_;

    void onDnsResolvedCallback(const DnsCacheResult &result);
    void onDnsResolvedImpl(const DnsCacheResult &result);

    void onCurlFinishedCallback(std::uint64_t requestId, std::shared_ptr<WSNetRequestError> error);
    void onCurlProgressCallback(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal);
    void onCurlReadyDataCallback(std::uint64_t requestId, const std::string &data);

    void onCurlFinishedCallbackImpl(std::uint64_t requestId, std::shared_ptr<WSNetRequestError> error);
    void onCurlProgressCallbackImpl(std::uint64_t requestId, std::uint64_t bytesReceived, std::uint64_t bytesTotal);
    void onCurlReadyDataCallbackImpl(std::uint64_t requestId, const std::string &data);

    void whitelistIps(const std::vector<std::string> &ips);
    void removeWhitelistIps(const std::vector<std::string> &ips);

    void cancelAndRemoveRequest(const std::map<std::uint64_t, RequestData>::iterator &request);
};

} // namespace wsnet
