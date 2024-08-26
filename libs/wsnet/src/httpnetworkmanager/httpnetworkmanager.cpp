#include "httpnetworkmanager.h"
#include "httprequest.h"
#include "utils/cancelablecallback.h"

namespace wsnet {

HttpNetworkManager::HttpNetworkManager(boost::asio::io_context &io_context, WSNetDnsResolver *dnsResolver) :
    io_context_(io_context), impl_(io_context, dnsResolver)
{
}

bool HttpNetworkManager::init()
{
    return impl_.init();
}

std::shared_ptr<WSNetHttpRequest> HttpNetworkManager::createGetRequest(const std::string &url, std::uint32_t timeoutMs, bool isIgnoreSslErrors)
{
    return std::make_shared<HttpRequest>(url, timeoutMs, HttpMethod::kGet, isIgnoreSslErrors);
}

std::shared_ptr<WSNetHttpRequest> HttpNetworkManager::createPostRequest(const std::string &url, std::uint32_t timeoutMs, const std::string &data, bool isIgnoreSslErrors)
{
    return std::make_shared<HttpRequest>(url, timeoutMs, HttpMethod::kPost, isIgnoreSslErrors, data);
}

std::shared_ptr<WSNetHttpRequest> HttpNetworkManager::createPutRequest(const std::string &url, std::uint32_t timeoutMs, const std::string &data, bool isIgnoreSslErrors)
{
    return std::make_shared<HttpRequest>(url, timeoutMs, HttpMethod::kPut, isIgnoreSslErrors, data);
}

std::shared_ptr<WSNetHttpRequest> HttpNetworkManager::createDeleteRequest(const std::string &url, std::uint32_t timeoutMs, bool isIgnoreSslErrors)
{
    return std::make_shared<HttpRequest>(url, timeoutMs, HttpMethod::kDelete, isIgnoreSslErrors);
}

std::shared_ptr<WSNetCancelableCallback> HttpNetworkManager::executeRequestEx(const std::shared_ptr<WSNetHttpRequest> &request, std::uint64_t id, WSNetHttpNetworkManagerFinishedCallback finishedCallback,
                                                 WSNetHttpNetworkManagerProgressCallback progressCallback, WSNetHttpNetworkManagerReadyDataCallback readyReadCallback)
{
    auto cc = std::make_shared<HttpNetworkManagerCallbacks>(finishedCallback, progressCallback, readyReadCallback);
    boost::asio::post(io_context_, [this, request, id, cc] {
        impl_.executeRequest(request, id, cc);
    });
    return cc;
}

void HttpNetworkManager::setProxySettings(const std::string &address, const std::string &username, const std::string &password)
{
    boost::asio::post(io_context_, [this, address, username, password] {
        impl_.setProxySettings(address, username, password);
    });
}

std::shared_ptr<WSNetCancelableCallback> HttpNetworkManager::setWhitelistIpsCallback(WSNetHttpNetworkManagerWhitelistIpsCallback whitelistIpsCallback)
{
    if (whitelistIpsCallback) {
        auto cancelableCallback = std::make_shared<CancelableCallback<WSNetHttpNetworkManagerWhitelistIpsCallback>>(whitelistIpsCallback);
        boost::asio::post(io_context_, [this, cancelableCallback] {
            impl_.setWhitelistIpsCallback(cancelableCallback);
        });
        return cancelableCallback;
    } else {
        boost::asio::post(io_context_, [this] {
            impl_.setWhitelistIpsCallback(nullptr);
        });
        return nullptr;
    }
}

std::shared_ptr<WSNetCancelableCallback> HttpNetworkManager::setWhitelistSocketsCallback(WSNetHttpNetworkManagerWhitelistSocketsCallback whitelistSocketsCallback)
{
    if (whitelistSocketsCallback) {
        auto cancelableCallback = std::make_shared<CancelableCallback<WSNetHttpNetworkManagerWhitelistSocketsCallback>>(whitelistSocketsCallback);
        boost::asio::post(io_context_, [this, cancelableCallback] {
            impl_.setWhitelistSocketsCallback(cancelableCallback);
        });
        return cancelableCallback;
    } else {
        boost::asio::post(io_context_, [this] {
            impl_.setWhitelistSocketsCallback(nullptr);
        });
        return nullptr;
    }
}

} // namespace wsnet

