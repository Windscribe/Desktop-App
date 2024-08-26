#pragma once
#include <set>
#include <functional>
#include <memory>
#include "scapix_object.h"
#include "WSNetHttpRequest.h"
#include "WSNetCancelableCallback.h"


namespace wsnet {

//TODO: kTimeoutExceed remove?
enum class NetworkError { kSuccess, kTimeoutExceed, kDnsResolveError, kCurlError };

typedef std::function<void(std::uint64_t requestId, std::uint32_t elapsedMs,
                           NetworkError errCode, const std::string &data)> WSNetHttpNetworkManagerFinishedCallback;

typedef std::function<void(std::uint64_t requestId, std::uint64_t bytesReceived,
                           std::uint64_t bytesTotal)> WSNetHttpNetworkManagerProgressCallback;

typedef std::function<void(std::uint64_t requestId, const std::string &data)> WSNetHttpNetworkManagerReadyDataCallback;

typedef std::function<void(const std::set<std::string> &ips)> WSNetHttpNetworkManagerWhitelistIpsCallback;

typedef std::function<void(const std::set<int> &sockets)> WSNetHttpNetworkManagerWhitelistSocketsCallback;

// Some simplified implementation of HTTP network manager for our needs based on curl and custom DNS-resolver based on c-ares.
// In particular, it has the functionality to whitelist IP addresses to firewall exceptions.
// It has an internal DNS cache.
// It's thread safe, you can call class functions from any thread.
class WSNetHttpNetworkManager : public scapix_object<WSNetHttpNetworkManager>
{
public:
    virtual ~WSNetHttpNetworkManager() {}

    virtual std::shared_ptr<WSNetHttpRequest> createGetRequest(const std::string &url, std::uint32_t timeoutMs,
                                                               bool isIgnoreSslErrors = false) = 0;

    virtual std::shared_ptr<WSNetHttpRequest> createPostRequest(const std::string &url, std::uint32_t timeoutMs,
                                                                const std::string &data, bool isIgnoreSslErrors = false) = 0;

    virtual std::shared_ptr<WSNetHttpRequest> createPutRequest(const std::string &url, std::uint32_t timeoutMs,
                                                               const std::string &data, bool isIgnoreSslErrors = false) = 0;

    virtual std::shared_ptr<WSNetHttpRequest> createDeleteRequest(const std::string &url, std::uint32_t timeoutMs,
                                                                  bool isIgnoreSslErrors = false) = 0;

    std::shared_ptr<WSNetCancelableCallback> executeRequest(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t requestId,
                                                            WSNetHttpNetworkManagerFinishedCallback finishedCallback)
    {
        return executeRequestEx(request, requestId, finishedCallback, nullptr, nullptr);
    }

    virtual std::shared_ptr<WSNetCancelableCallback> executeRequestEx(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t requestId,
                                         WSNetHttpNetworkManagerFinishedCallback finishedCallback,
                                         WSNetHttpNetworkManagerProgressCallback progressCallback = nullptr,
                                         WSNetHttpNetworkManagerReadyDataCallback readyDataCallback = nullptr) = 0;

    // to not use a proxy all values must be empty
    // address example: https://1.2.3.4:8083, details: https://curl.se/libcurl/c/CURLOPT_PROXY.html
    virtual void setProxySettings(const std::string &address = std::string(),
                                  const std::string &username = std::string(),
                                  const std::string &password = std::string()) = 0;

    // callback function allowing the caller to add IP-addresses to the firewall exceptions
    // this callback function must return control after the firewall is configured
    // you can pass null to disable the callback function
    virtual std::shared_ptr<WSNetCancelableCallback> setWhitelistIpsCallback(WSNetHttpNetworkManagerWhitelistIpsCallback whitelistIpsCallback) = 0;

    // callback function allowing the caller to add sockets to the firewall exceptions (Android specific)
    // this callback function must return control after the firewall is configured
    // you can pass null to disable the callback function
    virtual std::shared_ptr<WSNetCancelableCallback> setWhitelistSocketsCallback(WSNetHttpNetworkManagerWhitelistSocketsCallback whitelistSocketsCallback) = 0;
};

} // namespace wsnet
