#ifndef CURLREPLY_H
#define CURLREPLY_H

#include <QRecursiveMutex>
#include "networkrequest.h"
#include <curl/curl.h>

class CurlNetworkManager2;

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

    explicit CurlReply(QObject *parent, const NetworkRequest &networkRequest, const QStringList &ips, REQUEST_TYPE requestType, const QByteArray &postData, CurlNetworkManager2 *manager);

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
    QStringList ips_;
    REQUEST_TYPE requestType_;
    QByteArray postData_;
    CurlNetworkManager2 *manager_;
    QVector<struct curl_slist *> curlLists_;


    friend class CurlNetworkManager2;    // CurlNetworkManager class uses private functions to store internal data
};



#endif // CURLREPLY_H
