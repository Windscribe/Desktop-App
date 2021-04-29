#include "networkaccessmanager.h"
#include "dnsresolver/dnsrequest.h"

std::atomic<quint64> NetworkAccessManager::nextId_ = 0;

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QObject(parent)
{
    curlNetworkManager_ = new CurlNetworkManager(this);
}

NetworkReply *NetworkAccessManager::get(const NetworkRequest &request)
{
    quint64 id = nextId_++;
    NetworkReply *reply = new NetworkReply(this);
    reply->setProperty("replyId", id);
    connect(reply, SIGNAL(destroyed(QObject*)), SLOT(onReplyDestroyed(QObject*)));

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

void NetworkAccessManager::handleRequest(quint64 id)
{
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
    else
    {
        Q_ASSERT(false);
    }
}

void NetworkAccessManager::onReplyDestroyed(QObject *obj)
{
    Q_ASSERT(obj->property("replyId") != QVariant::Invalid);
    quint64 id = obj->property("replyId").toULongLong();
    Q_ASSERT(activeRequests_.contains(id));
    activeRequests_.remove(id);
}

void NetworkAccessManager::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    Q_ASSERT(dnsRequest);

    quint64 replyId = dnsRequest->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end())
    {
        QSharedPointer<RequestData> requestData = it.value();



        //emit requestData->reply->finished();
    }
    dnsRequest->deleteLater();
}

NetworkReply::NetworkReply(QObject *parent) : QObject(parent)
{

}
