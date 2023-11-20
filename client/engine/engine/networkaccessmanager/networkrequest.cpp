#include "networkrequest.h"

NetworkRequest::NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache) : url_(url), timeout_(timeout), bUseDnsCache_(bUseDnsCache), bIgnoreSslErrors_(false),
    bRemoveFromWhitelistIpsAfterFinish_(false), isWhiteListIps_(true), bExtraTLSPadding_(false)
{
}

NetworkRequest::NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache, const QStringList &dnsServers, bool isIgnoreSslErrors) :
    url_(url),
    timeout_(timeout),
    bUseDnsCache_(bUseDnsCache),
    bIgnoreSslErrors_(isIgnoreSslErrors),
    dnsServers_(dnsServers),
    bRemoveFromWhitelistIpsAfterFinish_(false),
    isWhiteListIps_(true),
    bExtraTLSPadding_(false)
{
}

void NetworkRequest::setUrl(const QUrl &url)
{
    url_ = url;
}

QUrl NetworkRequest::url() const
{
    return url_;
}

void NetworkRequest::setTimeout(int timeout)
{
    timeout_ = timeout;
}

int NetworkRequest::timeout() const
{
    return timeout_;
}

void NetworkRequest::setUseDnsCache(bool bUseDnsCache)
{
    bUseDnsCache_ = bUseDnsCache;
}

bool NetworkRequest::isUseDnsCache() const
{
    return bUseDnsCache_;
}

void NetworkRequest::setDnsServers(const QStringList &dnsServers)
{
    dnsServers_ = dnsServers;
}

QStringList NetworkRequest::dnsServers() const
{
    return dnsServers_;
}

void NetworkRequest::setContentTypeHeader(const QString &header)
{
    header_ = header;
}

QString NetworkRequest::contentTypeHeader() const
{
    return header_;
}

void NetworkRequest::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

bool NetworkRequest::isIgnoreSslErrors() const
{
    return bIgnoreSslErrors_;
}

void NetworkRequest::setRemoveFromWhitelistIpsAfterFinish()
{
    bRemoveFromWhitelistIpsAfterFinish_ = true;
}

bool NetworkRequest::isRemoveFromWhitelistIpsAfterFinish() const
{
    return bRemoveFromWhitelistIpsAfterFinish_;
}

void NetworkRequest::setEchConfig(const QString &echConfig)
{
    echConfig_ = echConfig;
}

QString NetworkRequest::echConfig() const
{
    return echConfig_;
}

void NetworkRequest::setExtraTLSPadding(const bool ExtraTLSPadding)
{
    bExtraTLSPadding_ = ExtraTLSPadding;
}

bool NetworkRequest::isExtraTLSPadding() const
{
    return bExtraTLSPadding_;
}

void NetworkRequest::setOverrideIp(const QString &ip)
{
    overrideIp_ = ip;
}

QString NetworkRequest::overrideIp() const
{
    return overrideIp_;
}

void NetworkRequest::setIsWhiteListIps(bool isWhiteListIps)
{
    isWhiteListIps_ = isWhiteListIps;
}

bool NetworkRequest::isWhiteListIps() const
{
    return isWhiteListIps_;
}
