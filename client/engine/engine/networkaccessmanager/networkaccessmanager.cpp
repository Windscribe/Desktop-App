#include "networkaccessmanager.h"
#include "engine/dnsresolver/dnsrequest.h"

std::atomic<quint64> NetworkAccessManager::nextId_(0);

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QObject(parent)
{
    curlNetworkManager_ = new CurlNetworkManager2(this);
    dnsCache_ = new DnsCache2(this);
    connect(dnsCache_, SIGNAL(resolved(bool,QStringList,quint64,bool, int)), SLOT(onResolved(bool,QStringList,quint64,bool, int)));
    connect(dnsCache_, SIGNAL(whitelistIpsChanged(QSet<QString>)), SIGNAL(whitelistIpsChanged(QSet<QString>)));
}

NetworkAccessManager::~NetworkAccessManager()
{
}

NetworkReply *NetworkAccessManager::get(const NetworkRequest &request)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_GET, request, QByteArray());
}

NetworkReply *NetworkAccessManager::post(const NetworkRequest &request, const QByteArray &data)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_POST, request, data);
}

NetworkReply *NetworkAccessManager::put(const NetworkRequest &request, const QByteArray &data)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_PUT, request, data);
}

NetworkReply *NetworkAccessManager::deleteResource(const NetworkRequest &request)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_DELETE, request, QByteArray());
}

void NetworkAccessManager::abort(NetworkReply *reply)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    Q_ASSERT(reply->property("replyId").isValid());
    quint64 id = reply->property("replyId").toULongLong();

    auto it = activeRequests_.find(id);
    Q_ASSERT(it != activeRequests_.end());
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->reply->abortCurl();
        activeRequests_.erase(it);
    }

    dnsCache_->notifyFinished(id);
}

void NetworkAccessManager::handleRequest(quint64 id)
{
    Q_ASSERT(QThread::currentThread() == this->thread());

    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        QString hostname = requestData->request.url().host();
        dnsCache_->resolve(hostname, requestData->id, !requestData->request.isUseDnsCache(), requestData->request.dnsServers(), requestData->request.timeout());
    }
}

void NetworkAccessManager::onCurlReplyFinished()
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->reply->checkForCurlError();
        emit requestData->reply->finished();
        dnsCache_->notifyFinished(replyId);
    }
}

void NetworkAccessManager::onCurlProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        emit requestData->reply->progress(bytesReceived, bytesTotal);
    }
}

void NetworkAccessManager::onCurlReadyRead()
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        emit requestData->reply->readyRead();
    }

}

void NetworkAccessManager::onResolved(bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs)
{
    Q_UNUSED(bFromCache);
    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();

        if (success)
        {
            if (requestData->request.timeout() - timeMs > 0)
            {
                requestData->request.setTimeout(requestData->request.timeout() - timeMs);

                CurlReply *curlReply{ nullptr };

                if (requestData->type == REQUEST_GET)
                {
                    curlReply = curlNetworkManager_->get(requestData->request, ips);
                }
                else if (requestData->type == REQUEST_POST)
                {
                    curlReply = curlNetworkManager_->post(requestData->request, requestData->data, ips);
                }
                else if (requestData->type == REQUEST_PUT)
                {
                    curlReply = curlNetworkManager_->put(requestData->request, requestData->data, ips);
                }
                else if (requestData->type == REQUEST_DELETE)
                {
                    curlReply = curlNetworkManager_->deleteResource(requestData->request, ips);
                }
                else
                {
                    Q_ASSERT(false);
                }


                curlReply->setProperty("replyId", id);
                requestData->reply->setCurlReply(curlReply);

                connect(curlReply, SIGNAL(finished()), SLOT(onCurlReplyFinished()));
                connect(curlReply, SIGNAL(progress(qint64,qint64)), SLOT(onCurlProgress(qint64,qint64)));
                connect(curlReply, SIGNAL(readyRead()), SLOT(onCurlReadyRead()));
            }
            // timeout exceed
            else
            {
                requestData->reply->setError(NetworkReply::TimeoutExceed);
                emit requestData->reply->finished();
            }
        }
        else
        {
            requestData->reply->setError(NetworkReply::DnsResolveError);
            emit requestData->reply->finished();
        }
    }
}

quint64 NetworkAccessManager::getNextId()
{
    return nextId_++;
}

NetworkReply *NetworkAccessManager::invokeHandleRequest(NetworkAccessManager::REQUEST_TYPE type, const NetworkRequest &request, const QByteArray &data)
{
    quint64 id = getNextId();
    NetworkReply *reply = new NetworkReply(this);
    reply->setProperty("replyId", id);

    QSharedPointer<RequestData> requestData(new RequestData);
    requestData->id = id;
    requestData->type = type;
    requestData->request = request;
    requestData->reply = reply;
    requestData->data = data;

    Q_ASSERT(!activeRequests_.contains(id));
    activeRequests_[id] = requestData;

    QMetaObject::invokeMethod(this, "handleRequest", Qt::QueuedConnection, Q_ARG(quint64, id));

    return reply;
}

NetworkReply::NetworkReply(NetworkAccessManager *parent) : QObject(parent), curlReply_(NULL), manager_(parent), error_(NetworkReply::NoError)
{

}

NetworkReply::~NetworkReply()
{
    abort();
}

void NetworkReply::abort()
{
    manager_->abort(this);
    if (curlReply_)
    {
        curlReply_->deleteLater();
        curlReply_ = nullptr;
    }
}

QByteArray NetworkReply::readAll()
{
    if (curlReply_)
    {
        return curlReply_->readAll();
    }
    else
    {
        Q_ASSERT(false);
        return QByteArray();
    }
}

bool NetworkReply::isSuccess() const
{
    return error_ == NoError;
}

void NetworkReply::setCurlReply(CurlReply *curlReply)
{
    curlReply_ = curlReply;
}

void NetworkReply::abortCurl()
{
    if (curlReply_)
    {
        curlReply_->abort();
    }
}

void NetworkReply::checkForCurlError()
{
    if (curlReply_ && !curlReply_->isSuccess())
    {
        error_ = CurlError;
    }
}

void NetworkReply::setError(NetworkReply::NetworkError err)
{
    error_ = err;
}
