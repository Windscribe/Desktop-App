#include "curlreply.h"
#include "curlnetworkmanager.h"

CurlReply::CurlReply(QObject *parent, const NetworkRequest &networkRequest, const QStringList &ips, REQUEST_TYPE requestType, CurlNetworkManager *manager)
    : QObject(parent), mutex_(QMutex::Recursive), networkRequest_(networkRequest), ips_(ips), requestType_(requestType), manager_(manager)
{
    static std::atomic<quint64> id = 0;
    id_ = id++;
}

const NetworkRequest &CurlReply::networkRequest() const
{
    return networkRequest_;
}

QStringList CurlReply::ips() const
{
    return ips_;
}

void CurlReply::appendNewData(const QByteArray &newData)
{
    QMutexLocker locker(&mutex_);
    data_.append(newData);
    emit readyRead();
}

void CurlReply::setCurlErrorCode(int curlErrorCode)
{
    QMutexLocker locker(&mutex_);
    curlErrorCode_ = curlErrorCode;
}

CurlReply::REQUEST_TYPE CurlReply::requestType() const
{
    return requestType_;
}

quint64 CurlReply::id() const
{
    return id_;
}

CurlReply::~CurlReply()
{
    abort();
}

void CurlReply::abort()
{
    manager_->abort(this);
}

QByteArray CurlReply::readAll()
{
    QByteArray temp;
    {
        QMutexLocker locker(&mutex_);
        temp = data_;
        data_.clear();
    }
    return temp;
}

int CurlReply::curlErrorCode() const
{
    QMutexLocker locker(&mutex_);
    return curlErrorCode_;
}
