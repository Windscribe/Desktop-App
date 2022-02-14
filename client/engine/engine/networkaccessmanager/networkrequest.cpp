#include "networkrequest.h"

NetworkRequest::NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache) : url_(url), timeout_(timeout), bUseDnsCache_(bUseDnsCache), bIgnoreSslErrors_(false)
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

void NetworkRequest::setProxySettings(const ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

const ProxySettings &NetworkRequest::proxySettings() const
{
    return proxySettings_;
}
