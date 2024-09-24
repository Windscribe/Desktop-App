#pragma once
#include "WSNetHttpRequest.h"
#include <memory>

namespace wsnet {

class HttpRequest : public WSNetHttpRequest
{
public:
    HttpRequest(const std::string &url, std::uint32_t timeoutMs, HttpMethod httpMethod,
                bool isIgnoreSslErrors, const std::string &postData = std::string());

    virtual ~HttpRequest();

    std::string url() const override;
    std::uint32_t timeoutMs() const override;
    std::string postData() const override;
    HttpMethod method() const override;

    std::string hostname() const override;
    std::uint16_t port() const override;

    // true by default
    void setUseDnsCache(bool isUseDnsCache) override;
    bool isUseDnsCache() const override;

    // empty by default
    void setContentTypeHeader(const std::string &header) override;
    std::string contentTypeHeader() const override;

    // false by default
    void setIgnoreSslErrors(bool isIgnore) override;
    bool isIgnoreSslErrors() const override;

    // false by default
    void setRemoveFromWhitelistIpsAfterFinish(bool isRemove) override;
    bool isRemoveFromWhitelistIpsAfterFinish() const override;

    // empty by default
    void setEchConfig(const std::string &echConfig) override;
    std::string echConfig() const override;

    // empty by default
    void setSniDomain(const std::string &sniDomain) override;
    std::string sniDomain() const override;
    std::string sniUrl() const override;

    // false by default
    void setExtraTLSPadding(bool isExtraTLSPadding) override;
    bool isExtraTLSPadding() const override;

    // Explicitly specify ip to avoid DNS resolution
    // empty by default
    void setOverrideIp(const std::string &ip) override;
    std::string overrideIp() const override;

    // true by default
    void setIsWhiteListIps(bool isWhiteListIps) override;
    bool isWhiteListIps() const override;

    // false by default
    // makes additional logs through which IP the request was made and its curl error
    void setIsDebugLogCurlError(bool isEnabled) override;
    bool isDebugLogCurlError() const override;

private:
    // internal implementation class (to hide include skyr/url.hpp from this header, there were compilation errors in Windows)
    struct Impl;
    // pointer to the internal implementation
    std::unique_ptr<Impl> pImpl_;
};

} // namespace wsnet
