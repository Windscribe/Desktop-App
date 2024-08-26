#include "httprequest.h"
#include <skyr/url.hpp>

namespace wsnet {

struct HttpRequest::Impl
{
    std::string url;
    std::uint32_t timeoutMs;
    std::string postData;
    HttpMethod httpMethod;
    bool isUseDnsCache = true;
    std::string contentHeader;
    bool isIgnoreSslErrors = false;
    bool isRemoveFromWhitelistIps = false;
    std::string echConfig;
    std::string sniDomain;
    bool isExtraTLSPadding = false;
    std::string overrideIp;
    bool isWhiteListIps = true;
    skyr::url skyrUrl;
};

HttpRequest::HttpRequest(const std::string &url, std::uint32_t timeoutMs, HttpMethod httpMethod, bool isIgnoreSslErrors, const std::string &postData)
{
    pImpl_ = std::make_unique<Impl>();
    pImpl_->url = url;
    pImpl_->timeoutMs = timeoutMs;
    pImpl_->httpMethod = httpMethod;
    pImpl_->isIgnoreSslErrors = isIgnoreSslErrors;
    pImpl_->postData = postData;
    pImpl_->skyrUrl = skyr::url(url);
    assert(!pImpl_->skyrUrl.is_empty_host());
}

HttpRequest::~HttpRequest()
{
}

std::string HttpRequest::url() const
{
    return pImpl_->url;
}

std::uint32_t HttpRequest::timeoutMs() const
{
    return pImpl_->timeoutMs;
}

std::string HttpRequest::postData() const
{
    return pImpl_->postData;
}

HttpMethod HttpRequest::method() const
{
    return pImpl_->httpMethod;
}

std::string HttpRequest::hostname() const
{
    return pImpl_->skyrUrl.hostname();
}

std::uint16_t HttpRequest::port() const
{
    if (pImpl_->skyrUrl.port().empty())
        return 0;
    else
        return (std::uint16_t)std::stoul(pImpl_->skyrUrl.port());
}

void HttpRequest::setUseDnsCache(bool isUseDnsCache)
{
    pImpl_->isUseDnsCache = isUseDnsCache;
}

bool HttpRequest::isUseDnsCache() const
{
    return pImpl_->isUseDnsCache;
}

void HttpRequest::setContentTypeHeader(const std::string &header)
{
    pImpl_->contentHeader = header;
}

std::string HttpRequest::contentTypeHeader() const
{
    return pImpl_->contentHeader;
}

void HttpRequest::setIgnoreSslErrors(bool isIgnore)
{
    pImpl_->isIgnoreSslErrors = isIgnore;
}

bool HttpRequest::isIgnoreSslErrors() const
{
    return pImpl_->isIgnoreSslErrors;
}

void HttpRequest::setRemoveFromWhitelistIpsAfterFinish(bool isRemove)
{
    pImpl_->isRemoveFromWhitelistIps = isRemove;
}

bool HttpRequest::isRemoveFromWhitelistIpsAfterFinish() const
{
    return pImpl_->isRemoveFromWhitelistIps;
}

void HttpRequest::setEchConfig(const std::string &echConfig)
{
    pImpl_->echConfig = echConfig;
}

std::string HttpRequest::echConfig() const
{
    return pImpl_->echConfig;
}

void HttpRequest::setSniDomain(const std::string &sniDomain)
{
    pImpl_->sniDomain = sniDomain;
}

std::string HttpRequest::sniDomain() const
{
    return pImpl_->sniDomain;
}

std::string HttpRequest::sniUrl() const
{
    skyr::url u = pImpl_->skyrUrl;
    u.set_hostname(pImpl_->sniDomain);
    return u.c_str();
}

void HttpRequest::setExtraTLSPadding(bool isExtraTLSPadding)
{
    pImpl_->isExtraTLSPadding = isExtraTLSPadding;
}

bool HttpRequest::isExtraTLSPadding() const
{
    return pImpl_->isExtraTLSPadding;
}

void HttpRequest::setOverrideIp(const std::string &ip)
{
    pImpl_->overrideIp = ip;
}

std::string HttpRequest::overrideIp() const
{
    return pImpl_->overrideIp;
}

void HttpRequest::setIsWhiteListIps(bool isWhiteListIps)
{
    pImpl_->isWhiteListIps = isWhiteListIps;
}

bool HttpRequest::isWhiteListIps() const
{
    return pImpl_->isWhiteListIps;
}

} // namespace wsnet

