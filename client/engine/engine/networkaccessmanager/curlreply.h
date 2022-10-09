#pragma once

#include <QObject>
#include <QMutex>
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

    explicit CurlReply(CurlNetworkManager *manager, quint64 id);

    void appendNewData(const QByteArray &newData);
    void setCurlErrorCode(CURLcode curlErrorCode);
    quint64 id() const;

    QByteArray data_;
    mutable QRecursiveMutex mutex_;
    CURLcode curlErrorCode_;
    quint64 id_;
    CurlNetworkManager *manager_;


    friend class CurlNetworkManager;    // CurlNetworkManager class uses private functions to store internal data
};



