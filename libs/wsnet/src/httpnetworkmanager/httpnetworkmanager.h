#pragma once

#include "WSNetHttpNetworkManager.h"
#include <BS_thread_pool.hpp>
#include "httpnetworkmanager_impl.h"

namespace wsnet {

// Essentially redirects all calls to the HttpNetworkManager_impl for execution on the library thread (task queue)
class HttpNetworkManager : public WSNetHttpNetworkManager
{
public:
    HttpNetworkManager(BS::thread_pool &taskQueue, WSNetDnsResolver *dnsResolver);

    bool init();

    std::shared_ptr<WSNetHttpRequest> createGetRequest(const std::string &url, std::uint16_t timeoutMs,
                                                       bool isIgnoreSslErrors = false) override;

    std::shared_ptr<WSNetHttpRequest> createPostRequest(const std::string &url, std::uint16_t timeoutMs,
                                                        const std::string &data, bool isIgnoreSslErrors = false) override;

    std::shared_ptr<WSNetHttpRequest> createPutRequest(const std::string &url, std::uint16_t timeoutMs,
                                                       const std::string &data, bool isIgnoreSslErrors = false) override;

    std::shared_ptr<WSNetHttpRequest> createDeleteRequest(const std::string &url, std::uint16_t timeoutMs,
                                                          bool isIgnoreSslErrors = false) override;

    std::shared_ptr<WSNetCancelableCallback> executeRequestEx(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t id,
                                         WSNetHttpNetworkManagerFinishedCallback finishedCallback,
                                         WSNetHttpNetworkManagerProgressCallback progressCallback,
                                         WSNetHttpNetworkManagerReadyDataCallback readyReadCallback) override;

    void setProxySettings(const std::string &address = std::string(),
                          const std::string &username = std::string(),
                          const std::string &password = std::string()) override;

    std::shared_ptr<WSNetCancelableCallback> setWhitelistIpsCallback(WSNetHttpNetworkManagerWhitelistIpsCallback whitelistIpsCallback) override;

private:
    BS::thread_pool &taskQueue_;
    HttpNetworkManager_impl impl_;
};

} // namespace wsnet
