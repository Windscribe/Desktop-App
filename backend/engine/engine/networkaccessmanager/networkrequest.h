#ifndef NETWORKREQUEST_H
#define NETWORKREQUEST_H

#include <QUrl>
#include "proxy/proxysettings.h"

class NetworkRequest
{
public:
    explicit NetworkRequest() {}
    explicit NetworkRequest(const QUrl &url, int timeout, bool bUseDnsCache);

    void setUrl(const QUrl &url);
    QUrl url() const;

    void setTimeout(int timeout);
    int timeout() const;

    void setUseDnsCache(bool bUseDnsCache);
    bool isUseDnsCache() const;

    void setContentTypeHeader(const QString &header);
    QString contentTypeHeader() const;

    void setIgnoreSslErrors(bool bIgnore);
    bool isIgnoreSslErrors() const;

    void setProxySettings(const ProxySettings &proxySettings);
    const ProxySettings &proxySettings() const;

private:
    QUrl url_;
    int timeout_;
    bool bUseDnsCache_;
    ProxySettings proxySettings_;
    bool bIgnoreSslErrors_;
    QString header_;
};

#endif // NETWORKREQUEST_H
