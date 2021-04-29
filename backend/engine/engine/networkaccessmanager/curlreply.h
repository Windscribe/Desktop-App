#ifndef CURLREPLY_H
#define CURLREPLY_H

#include <QMutex>
#include "networkrequest.h"

class CurlNetworkManager;

class CurlReply : public QObject
{
    Q_OBJECT

public:
    virtual ~CurlReply();

    void abort();
    QByteArray readAll();
    int curlErrorCode() const;

signals:
    void finished();
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void readyRead();

private:
    enum REQUEST_TYPE { REQUEST_GET, REQUEST_POST, REQUEST_PUT, REQUEST_DELETE};

    explicit CurlReply(QObject *parent, const NetworkRequest &networkRequest, const QStringList &ips, REQUEST_TYPE requestType, CurlNetworkManager *manager);

    const NetworkRequest &networkRequest() const;
    QStringList ips() const;
    void appendNewData(const QByteArray &newData);
    void setCurlErrorCode(int curlErrorCode);
    REQUEST_TYPE requestType() const;

    quint64 id() const;

    QByteArray data_;
    mutable QMutex mutex_;
    int curlErrorCode_;

    NetworkRequest networkRequest_;
    QStringList ips_;
    quint64 id_;

    REQUEST_TYPE requestType_;
    CurlNetworkManager *manager_;

    friend class CurlNetworkManager;    // CurlNetworkManager class uses private functions to store internal data
};



#endif // CURLREPLY_H
