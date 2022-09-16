#include "networkaccessmanager.h"
#include "utils/ws_assert.h"

std::atomic<quint64> NetworkAccessManager::nextId_(0);

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QObject(parent)
{
    curlNetworkManager_ = new CurlNetworkManager2(this);
    dnsCache_ = new DnsCache2(this);
    connect(dnsCache_, &DnsCache2::resolved,  this, &NetworkAccessManager::onResolved);
    connect(dnsCache_, &DnsCache2::whitelistIpsChanged, [this] (const QSet<QString> &ips) {
        emit whitelistIpsChanged(ips);
    });
}

NetworkAccessManager::~NetworkAccessManager()
{
    // Delete the NetworkReply children first.
    // This is important since they have access to manager in their destructor.
    const QList<NetworkReply *> replies = findChildren<NetworkReply *>();
    for (auto it : replies)
        delete it;
}

NetworkReply *NetworkAccessManager::get(const NetworkRequest &request)
{
    WS_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_GET, request, QByteArray());
}

NetworkReply *NetworkAccessManager::post(const NetworkRequest &request, const QByteArray &data)
{
    WS_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_POST, request, data);
}

NetworkReply *NetworkAccessManager::put(const NetworkRequest &request, const QByteArray &data)
{
    WS_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_PUT, request, data);
}

NetworkReply *NetworkAccessManager::deleteResource(const NetworkRequest &request)
{
    WS_ASSERT(QThread::currentThread() == this->thread());
    return invokeHandleRequest(REQUEST_DELETE, request, QByteArray());
}

void NetworkAccessManager::abort(NetworkReply *reply)
{
    WS_ASSERT(QThread::currentThread() == this->thread());
    WS_ASSERT(reply->property("replyId").isValid());
    quint64 id = reply->property("replyId").toULongLong();

    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->reply->abortCurl();
        activeRequests_.erase(it);
    }

    dnsCache_->notifyFinished(id);
}

void NetworkAccessManager::handleRequest(quint64 id)
{
    WS_ASSERT(QThread::currentThread() == this->thread());

    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        QString hostname = requestData->request.url().host();
        dnsCache_->resolve(hostname, requestData->id, !requestData->request.isUseDnsCache(), requestData->request.dnsServers(), requestData->request.timeout());
    }
}

void NetworkAccessManager::onCurlReplyFinished()
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->reply->checkForCurlError();
        emit requestData->reply->finished();
        dnsCache_->notifyFinished(replyId);
        activeRequests_.erase(it);
    }
}

void NetworkAccessManager::onCurlProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        emit requestData->reply->progress(bytesReceived, bytesTotal);
    }
}

void NetworkAccessManager::onCurlReadyRead()
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        emit requestData->reply->readyRead();
    }
}

void NetworkAccessManager::onResolved(bool success, const QStringList &ips, quint64 id, bool bFromCache, int timeMs)
{
    Q_UNUSED(bFromCache);
    const auto it = activeRequests_.find(id);
    if (it != activeRequests_.constEnd()) {
        QSharedPointer<RequestData> requestData = it.value();

        if (success) {
            if (requestData->request.timeout() - timeMs > 0) {
                requestData->request.setTimeout(requestData->request.timeout() - timeMs);

                CurlReply *curlReply{ nullptr };

                if (requestData->type == REQUEST_GET)
                    curlReply = curlNetworkManager_->get(requestData->request, ips);
                else if (requestData->type == REQUEST_POST)
                    curlReply = curlNetworkManager_->post(requestData->request, requestData->data, ips);
                else if (requestData->type == REQUEST_PUT)
                    curlReply = curlNetworkManager_->put(requestData->request, requestData->data, ips);
                else if (requestData->type == REQUEST_DELETE)
                    curlReply = curlNetworkManager_->deleteResource(requestData->request, ips);
                else
                    WS_ASSERT(false);

                curlReply->setProperty("replyId", id);
                requestData->reply->setCurlReply(curlReply);

                connect(curlReply, &CurlReply::finished, this, &NetworkAccessManager::onCurlReplyFinished);
                connect(curlReply, &CurlReply::progress, this, &NetworkAccessManager::onCurlProgress);
                connect(curlReply, &CurlReply::readyRead, this, &NetworkAccessManager::onCurlReadyRead);
            } else {    // timeout exceed
                requestData->reply->setError(NetworkReply::TimeoutExceed);
                emit requestData->reply->finished();
                activeRequests_.erase(it);
            }
        } else {
            requestData->reply->setError(NetworkReply::DnsResolveError);
            emit requestData->reply->finished();
            activeRequests_.erase(it);
        }
    } else {
        WS_ASSERT(false);
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

    WS_ASSERT(!activeRequests_.contains(id));
    activeRequests_[id] = requestData;

    QMetaObject::invokeMethod(this, "handleRequest", Qt::QueuedConnection, Q_ARG(quint64, id));

    return reply;
}

