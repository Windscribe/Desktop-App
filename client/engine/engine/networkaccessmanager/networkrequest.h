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

    void setEchConfig(const QString &echConfig);
    QString echConfig() const;
    void setSniDomain(const QString &sniDomain);
    QString sniDomain() const;

    void setExtraTLSPadding(const bool ExtraTLSPadding);
    bool isExtraTLSPadding() const;

    // Explicitly specify ip to avoid DNS resolution
    void setOverrideIp(const QString &ip);
    QString overrideIp() const;

    void setIsWhiteListIps(bool isWhiteListIps);
    bool isWhiteListIps() const;

private:
    QUrl url_;
    int timeout_;
    bool bUseDnsCache_;
    bool bIgnoreSslErrors_;
    QString header_;
    QStringList dnsServers_;

    QString sniDomain_;
    QString echConfig_;         // if not empty, use ECH request

    // if not empty use specified overrideIp_ to make the request
    QString overrideIp_;

    // default false, if true then immediately removes the IP from the whitelist ips after the request is completed.
    bool bRemoveFromWhitelistIpsAfterFinish_;

    // default true
    bool isWhiteListIps_;

    bool bExtraTLSPadding_;
};

