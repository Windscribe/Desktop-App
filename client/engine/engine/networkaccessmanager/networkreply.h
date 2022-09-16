#pragma once
#include <QObject>

class NetworkAccessManager;
class CurlReply;

class NetworkReply : public QObject
{
    Q_OBJECT
public:
    virtual ~NetworkReply();

    enum NetworkError { NoError, TimeoutExceed, DnsResolveError, SslError, CurlError };

    void abort();
    QByteArray readAll();
    NetworkError error() const;
    QString errorString() const;
    bool isSuccess() const;

signals:
    void finished();
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void readyRead();

private:
    explicit NetworkReply(NetworkAccessManager *parent);

    void setCurlReply(CurlReply *curlReply);
    void abortCurl();
    void checkForCurlError();

    void setError(NetworkError err);

    CurlReply *curlReply_;
    NetworkAccessManager *manager_;
    NetworkError error_;
    QString errorString_;

    friend class NetworkAccessManager;
};

