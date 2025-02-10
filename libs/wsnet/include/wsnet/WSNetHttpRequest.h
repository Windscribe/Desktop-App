#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "scapix_object.h"

namespace wsnet {

enum class HttpMethod {
    kGet = 0,
    kPost,
    kPut,
    kDelete
};

class WSNetHttpRequest : public scapix_object<WSNetHttpRequest>
{
public:
    virtual ~WSNetHttpRequest() {}

    virtual std::string url() const = 0;
    virtual std::uint32_t timeoutMs() const = 0;
    virtual std::string postData() const = 0;
    virtual HttpMethod method() const = 0;
    virtual std::string hostname() const = 0;
    virtual std::uint16_t port() const = 0;

    // true by default
    virtual void setUseDnsCache(bool isUseDnsCache) = 0;
    virtual bool isUseDnsCache() const = 0;

    // empty by default
    virtual void addHttpHeader(const std::string &header) = 0;
    virtual std::vector<std::string> httpHeaders() const = 0;

    // false by default
    virtual void setIgnoreSslErrors(bool isIgnore) = 0;
    virtual bool isIgnoreSslErrors() const = 0;

    // false by default
    virtual void setRemoveFromWhitelistIpsAfterFinish(bool isRemove) = 0;
    virtual bool isRemoveFromWhitelistIpsAfterFinish() const = 0;

    // empty by default
    virtual void setEchConfig(const std::string &echConfig) = 0;
    virtual std::string echConfig() const = 0;

    // empty by default
    virtual void setSniDomain(const std::string &sniDomain) = 0;
    virtual std::string sniDomain() const = 0;
    virtual std::string sniUrl() const = 0;

    // false by default
    virtual void setExtraTLSPadding(bool isExtraTLSPadding) = 0;
    virtual bool isExtraTLSPadding() const = 0;

    // Explicitly specify ip to avoid DNS resolution
    // empty by default
    virtual void setOverrideIp(const std::string &ip) = 0;
    virtual std::string overrideIp() const = 0;

    // true by default
    virtual void setIsWhiteListIps(bool isWhiteListIps) = 0;
    virtual bool isWhiteListIps() const = 0;

    // false by default
    // makes additional logs through which IP the request was made and its curl error
    virtual void setIsDebugLogCurlError(bool isEnabled) = 0;
    virtual bool isDebugLogCurlError() const = 0;
};

} // namespace wsnet
