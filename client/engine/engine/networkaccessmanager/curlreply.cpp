#include "curlreply.h"
#include "curlnetworkmanager.h"

CurlReply::CurlReply(CurlNetworkManager *manager, quint64 id)
    : QObject(manager),
      manager_(manager),
      id_(id)
{
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

