#pragma once

#include <QObject>
#include <QUrl>
#include "curlnetworkmanager.h"
#include "dnscache.h"
#include "networkreply.h"
#include "whitelistipsmanager.h"

// Some simplified implementation of the QNetworkAccessManager class for our needs based on curl and cares(DnsRequest).
// In particular, it has the functionality to whitelist IP addresses to firewall exceptions.
// It has an internal DNS cache.
class NetworkAccessManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkAccessManager(QObject *parent = nullptr);
    virtual ~NetworkAccessManager();

    NetworkReply *get(const NetworkRequest &request);
    NetworkReply *post(const NetworkRequest &request, const QByteArray &data);
    NetworkReply *put(const NetworkRequest &request, const QByteArray &data);
    NetworkReply *deleteResource(const NetworkRequest &request);

    void abort(NetworkReply *reply);

    void setProxySettings(const types::ProxySettings &proxySettings);
    void enableProxy();
    void disableProxy();

signals:
    // need for add exception rules to firewall
    // use only direct connection type, because the IPs must be resolved before the HTTP/HTTPS request is actually executed
    void whitelistIpsChanged(const QSet<QString> &ips);

private slots:
    void handleRequest(quint64 id);

    void onCurlReplyFinished();
    void onCurlProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onCurlReadyRead();

    void onResolved(bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs);

private:
    static std::atomic<quint64> nextId_;

    enum REQUEST_TYPE { REQUEST_GET, REQUEST_POST, REQUEST_PUT, REQUEST_DELETE};
    struct RequestData
    {
        quint64 id;
        REQUEST_TYPE type;
        NetworkRequest request;
        NetworkReply *reply;
        QByteArray data;
        QElapsedTimer elapsedTimer_;
    };

    QHash<quint64, QSharedPointer<RequestData> > activeRequests_;
    CurlNetworkManager *curlNetworkManager_;
    DnsCache *dnsCache_;
    WhitelistIpsManager *whitelistIpsManager_;
    types::ProxySettings proxySettings_;
    bool isProxyEnabled_ = true;

    types::ProxySettings currentProxySettings() const;
    quint64 getNextId();
    NetworkReply *invokeHandleRequest(REQUEST_TYPE type, const NetworkRequest &request, const QByteArray &data);
};

