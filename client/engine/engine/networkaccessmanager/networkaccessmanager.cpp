#include "networkaccessmanager.h"

#include <QSslSocket>
#include <QThread>

#include "utils/ws_assert.h"

namespace {
std::atomic<quint64> g_countInstances = 0;
}

std::atomic<quint64> NetworkAccessManager::nextId_(0);

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QObject(parent)
{
    //WS_ASSERT(g_countInstances == 0);       // this instance of the class is supposed to be a single instance for the entire program
    g_countInstances++;

    if (QSslSocket::supportsSsl())
        qCDebug(LOG_NETWORK) << "SSL version:" << QSslSocket::sslLibraryVersionString();
    else
        qCDebug(LOG_NETWORK) << "Fatal: SSL not supported";

    curlNetworkManager_ = new CurlNetworkManager(this);
    dnsCache_ = new DnsCache(this);
    connect(dnsCache_, &DnsCache::resolved,  this, &NetworkAccessManager::onResolved);

    whitelistIpsManager_ = new WhitelistIpsManager(this);
    connect(whitelistIpsManager_, &WhitelistIpsManager::whitelistIpsChanged, [this](const QSet<QString> &ips) {
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
    g_countInstances--;
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

        if (requestData->request.isRemoveFromWhitelistIpsAfterFinish())
            whitelistIpsManager_->remove(requestData->ips);
    }
}

void NetworkAccessManager::setProxySettings(const types::ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

void NetworkAccessManager::enableProxy()
{
    isProxyEnabled_ = true;
}

void NetworkAccessManager::disableProxy()
{
    isProxyEnabled_ = false;
}

void NetworkAccessManager::handleRequest(quint64 id)
{
    WS_ASSERT(QThread::currentThread() == this->thread());

    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();

        // skip DNS-resolution if the overrideIp is settled
        if (!requestData->request.overrideIp().isEmpty()) {
            onResolved(true, QStringList() << requestData->request.overrideIp(), requestData->id, false, 0);
        } else {
            QString hostname = requestData->request.url().host();
            if (!requestData->request.sniDomain().isEmpty()) {
                hostname = requestData->request.sniDomain();
            }
            dnsCache_->resolve(hostname, requestData->id, !requestData->request.isUseDnsCache(), requestData->request.dnsServers(), requestData->request.timeout());
        }
    }
}

void NetworkAccessManager::onCurlReplyFinished(qint64 elapsedMs)
{
    quint64 replyId = sender()->property("replyId").toULongLong();
    auto it = activeRequests_.find(replyId);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        activeRequests_.erase(it);
        requestData->elapsedMs_ += elapsedMs;
        requestData->reply->checkForCurlError();
        if (requestData->request.isRemoveFromWhitelistIpsAfterFinish())
            whitelistIpsManager_->remove(requestData->ips);
        emit requestData->reply->finished(requestData->elapsedMs_);
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
    auto it = activeRequests_.find(id);
    if (it != activeRequests_.end()) {
        QSharedPointer<RequestData> requestData = it.value();
        requestData->elapsedMs_ += timeMs;

        if (success) {
            if (requestData->request.timeout() - timeMs > 0) {
                requestData->request.setTimeout(requestData->request.timeout() - timeMs);
                requestData->ips = ips;

                if (requestData->request.isWhiteListIps())
                    whitelistIpsManager_->add(ips);

                CurlReply *curlReply{ nullptr };

                if (requestData->type == REQUEST_GET)
                    curlReply = curlNetworkManager_->get(requestData->request, ips, currentProxySettings());
                else if (requestData->type == REQUEST_POST)
                    curlReply = curlNetworkManager_->post(requestData->request, requestData->data, ips, currentProxySettings());
                else if (requestData->type == REQUEST_PUT)
                    curlReply = curlNetworkManager_->put(requestData->request, requestData->data, ips, currentProxySettings());
                else if (requestData->type == REQUEST_DELETE)
                    curlReply = curlNetworkManager_->deleteResource(requestData->request, ips, currentProxySettings());
                else
                    WS_ASSERT(false);

                curlReply->setProperty("replyId", id);
                requestData->reply->setCurlReply(curlReply);

                connect(curlReply, &CurlReply::finished, this, &NetworkAccessManager::onCurlReplyFinished);
                connect(curlReply, &CurlReply::progress, this, &NetworkAccessManager::onCurlProgress);
                connect(curlReply, &CurlReply::readyRead, this, &NetworkAccessManager::onCurlReadyRead);
            } else {    // timeout exceed
                activeRequests_.erase(it);
                requestData->reply->setError(NetworkReply::TimeoutExceed);
                emit requestData->reply->finished(requestData->elapsedMs_);
            }
        } else {
            activeRequests_.erase(it);
            requestData->reply->setError(NetworkReply::DnsResolveError);
            emit requestData->reply->finished(requestData->elapsedMs_);
        }
    }
}

types::ProxySettings NetworkAccessManager::currentProxySettings() const
{
    if (isProxyEnabled_)
        return proxySettings_;
    else
        return types::ProxySettings();
}

NetworkReply *NetworkAccessManager::invokeHandleRequest(NetworkAccessManager::REQUEST_TYPE type, const NetworkRequest &request, const QByteArray &data)
{
    quint64 id = nextId_++;

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
