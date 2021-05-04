#include "networkaccessmanager.h"
#include "dnsresolver/dnsrequest.h"

std::atomic<quint64> NetworkAccessManager::nextId_ = 0;

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QObject(parent)
{
    curlNetworkManager_ = new CurlNetworkManager(this);
}

NetworkReply *NetworkAccessManager::get(const NetworkRequest &request)
{
    Q_ASSERT(QThread::currentThread() == this->thread());

    quint64 id = nextId_++;
    NetworkReply *reply = new NetworkReply(this);
    reply->setProperty("replyId", id);

    QSharedPointer<RequestData> requestData(new RequestData);
    requestData->id = id;
    requestData->type = REQUEST_GET;
    requestData->request = request;
    requestData->reply = reply;

    Q_ASSERT(!activeRequests_.contains(id));
    activeRequests_[id] = requestData;

    QMetaObject::invokeMethod(this, "handleRequest", Qt::QueuedConnection, Q_ARG(quint64, id));

    return reply;
}

void NetworkAccessManager::abort(NetworkReply *reply)
{
    Q_ASSERT(QThread::currentThread() == this->thread());
    Q_ASSERT(reply->property("replyId") != QVariant::Invalid);
    quint64 id = reply->property("replyId").toULongLong();

    auto it = activeRequests_.find(id);
    Q_ASSERT(it != activeRequests_.end());
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->reply->abortCurl();
        activeRequests_.erase(it);
    }
}

void NetworkAccessManager::handleRequest(quint64 id)
{
    Q_ASSERT(QThread::currentThread() == this->thread());

    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        QString hostname = requestData->request.url().host();
        DnsRequest *dnsRequest = new DnsRequest(this, hostname);
        dnsRequest->setProperty("replyId", requestData->id);
        connect(dnsRequest, SIGNAL(finished()), SLOT(onDnsRequestFinished()));
        dnsRequest->lookup();
    }
}

void NetworkAccessManager::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    Q_ASSERT(dnsRequest);

    quint64 replyId = dnsRequest->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSet<QString> whitelistIps = lastWhitelistIps_;
        QSharedPointer<RequestData> requestData = it.value();

        if (!dnsRequest->isError())
        {
            for (auto ip : dnsRequest->ips())
            {
                whitelistIps.insert(ip);
            }

            if (whitelistIps != lastWhitelistIps_)
            {
                emit whitelistIpsChanged(whitelistIps);
                lastWhitelistIps_ = whitelistIps;
            }

            CurlReply *curlReply = curlNetworkManager_->get(requestData->request, dnsRequest->ips());
            curlReply->setProperty("replyId", replyId);
            requestData->reply->setCurlReply(curlReply);

            connect(curlReply, SIGNAL(finished()), SLOT(onCurlReplyFinished()));
            connect(curlReply, SIGNAL(progress(qint64,qint64)), SLOT(onCurlProgress(qint64,qint64)));
            connect(curlReply, SIGNAL(readyRead()), SLOT(onCurlReadyRead()));
        }
        else
        {

        }
    }
    dnsRequest->deleteLater();
}

void NetworkAccessManager::onCurlReplyFinished()
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();
        emit requestData->reply->finished();
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

NetworkReply::NetworkReply(NetworkAccessManager *parent) : QObject(parent), curlReply_(NULL), manager_(parent)
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
    if (curlReply_)
    {
        return curlReply_->isSuccess();
    }
    else
    {
        return false;
    }
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
