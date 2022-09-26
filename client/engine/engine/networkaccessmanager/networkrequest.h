#pragma once

#include <QUrl>

class NetworkRequest
{
public:
    NetworkRequest() {}
    explicit NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache);
    explicit NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache, const QStringList &dnsServers, bool isIgnoreSslErrors);

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

    void setRemoveFromWhitelistIpsAfterFinish();
    bool isRemoveFromWhitelistIpsAfterFinish() const;

private:
    QUrl url_;
    int timeout_;
    bool bUseDnsCache_;
    bool bIgnoreSslErrors_;
    QString header_;
    QStringList dnsServers_;

    //default false, if true then immediately removes the IP from the whitelist ips after the request is completed.
    bool bRemoveFromWhitelistIpsAfterFinish_;
};

