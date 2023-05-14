#pragma once

#include <QObject>
#include "curlreply.h"
#include "networkrequest.h"
#include "types/proxysettings.h"

class CurlNetworkManagerImpl;

// Not thread safe. Don't use it directly, use NetworkAccessManager instead.
class CurlNetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit CurlNetworkManager(QObject *parent = 0);
    virtual ~CurlNetworkManager();
    CurlReply *get(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings = types::ProxySettings());
    CurlReply *post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings = types::ProxySettings());
    CurlReply *put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings  = types::ProxySettings());
    CurlReply *deleteResource(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings  = types::ProxySettings());

    void abort(CurlReply *reply);

private slots:
    // this slots must be queued connected
    void onRequestFinished(quint64 requestId, CURLcode curlErrorCode, qint64 elapsedMs);
    void onRequestProgress(quint64 requestId, qint64 bytesReceived, qint64 bytesTotal);
    void onRequestNewData(quint64 requestId, const QByteArray &newData);

private:
    QHash<quint64, CurlReply *> activeRequests_;
    CurlNetworkManagerImpl *curlNetworkManagerImpl_;
};

