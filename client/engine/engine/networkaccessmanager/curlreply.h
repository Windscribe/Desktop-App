#pragma once

#include <QMutex>
#include "types/proxysettings.h"
#include "networkrequest.h"
#include <curl/curl.h>

class CurlNetworkManager;

class CurlReply : public QObject
{
    Q_OBJECT

public:
    virtual ~CurlReply();

    void abort();
    QByteArray readAll();
    bool isSSLError() const;
    bool isSuccess() const;
    QString errorString() const;

signals:
    void finished();
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void readyRead();

private:
    enum REQUEST_TYPE { REQUEST_GET, REQUEST_POST, REQUEST_PUT, REQUEST_DELETE};

    explicit CurlReply(CurlNetworkManager *manager, const NetworkRequest &networkRequest,
                       const types::ProxySettings &proxySettings, const QStringList &ips, REQUEST_TYPE requestType, const QByteArray &postData);

    const NetworkRequest &networkRequest() const;
    QStringList ips() const;
    void appendNewData(const QByteArray &newData);
    void setCurlErrorCode(CURLcode curlErrorCode);
    REQUEST_TYPE requestType() const;
    const QByteArray &postData() const;

    quint64 id() const;
    void addCurlListForFreeLater(struct curl_slist *list);

    QByteArray data_;
    mutable QRecursiveMutex mutex_;
    CURLcode curlErrorCode_;

    quint64 id_;
    NetworkRequest networkRequest_;
    types::ProxySettings proxySettings_;
    QStringList ips_;
    REQUEST_TYPE requestType_;
    QByteArray postData_;
    CurlNetworkManager *manager_;
    QVector<struct curl_slist *> curlLists_;


    friend class CurlNetworkManager;    // CurlNetworkManager class uses private functions to store internal data
};



