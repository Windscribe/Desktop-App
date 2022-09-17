#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include <QUrl>
#include "types/proxysettings.h"

class NetworkRequest
{
public:
    NetworkRequest() {}
    explicit NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache);
    explicit NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache, const QStringList &dnsServers, bool isIgnoreSslErrors, const types::ProxySettings &proxySettings);

    void setUrl(const QUrl &url);
    QUrl url() const;

    void setTimeout(int timeout);
    int timeout() const;

    void setUseDnsCache(bool bUseDnsCache);
    bool isUseDnsCache() const;

    void setDnsServers(const QStringList &dnsServers);
    QStringList dnsServers() const;

    void setContentTypeHeader(const QString &header);
    QString contentTypeHeader() const;

    void setIgnoreSslErrors(bool bIgnore);
    bool isIgnoreSslErrors() const;

    void setProxySettings(const types::ProxySettings &proxySettings);
    const types::ProxySettings &proxySettings() const;

    void setForceRemoveFromDnsCache();
    bool isForceRemoveFromDnsCache() const;

private:
    QUrl url_;
    int timeout_;
    bool bUseDnsCache_;
    types::ProxySettings proxySettings_;
    bool bIgnoreSslErrors_;
    QString header_;
    QStringList dnsServers_;

    //default false, if true then immediately removes the IP from the cache after the request is completed. This also leads to the removal of the IP from the firewall rules
    bool bForceRemoveFromDnsCache_;
};

#endif // NETWORKREQUEST_H
