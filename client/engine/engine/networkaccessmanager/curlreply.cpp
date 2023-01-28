#include "curlreply.h"
#include "curlnetworkmanager2.h"

CurlReply::CurlReply(QObject *parent, const NetworkRequest &networkRequest, const QStringList &ips, REQUEST_TYPE requestType, const QByteArray &postData, CurlNetworkManager2 *manager)
    : QObject(parent), mutex_(QRecursiveMutex()), networkRequest_(networkRequest), ips_(ips), requestType_(requestType), postData_(postData), manager_(manager)
{
    static std::atomic<quint64> id(0);
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

void CurlReply::setCurlErrorCode(CURLcode curlErrorCode)
{
    QMutexLocker locker(&mutex_);
    curlErrorCode_ = curlErrorCode;
}

CurlReply::REQUEST_TYPE CurlReply::requestType() const
{
    return requestType_;
}

const QByteArray &CurlReply::postData() const
{
    return postData_;
}

quint64 CurlReply::id() const
{
    return id_;
}

void CurlReply::addCurlListForFreeLater(curl_slist *list)
{
    curlLists_ << list;
}

CurlReply::~CurlReply()
{
    abort();
}

void CurlReply::abort()
{
    manager_->abort(this);
    for (struct curl_slist *list : qAsConst(curlLists_))
    {
        curl_slist_free_all(list);
    }
    curlLists_.clear();
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

bool CurlReply::isSSLError() const
{
    QMutexLocker locker(&mutex_);
    if (curlErrorCode_ == CURLE_SSL_CONNECT_ERROR || curlErrorCode_ == CURLE_SSL_CERTPROBLEM || curlErrorCode_ == CURLE_SSL_CIPHER ||
        curlErrorCode_ == CURLE_SSL_ISSUER_ERROR || curlErrorCode_ == CURLE_SSL_PINNEDPUBKEYNOTMATCH || curlErrorCode_ == CURLE_SSL_INVALIDCERTSTATUS ||
        curlErrorCode_ == CURLE_SSL_CACERT || curlErrorCode_ == CURLE_PEER_FAILED_VERIFICATION)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CurlReply::isSuccess() const
{
    QMutexLocker locker(&mutex_);
    return curlErrorCode_ == CURLE_OK;
}

QString CurlReply::errorString() const
{
    QMutexLocker locker(&mutex_);
    QString str(curl_easy_strerror(curlErrorCode_));
    return str;
}

