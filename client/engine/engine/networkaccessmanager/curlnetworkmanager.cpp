#include "curlnetworkmanager.h"

#include "curlnetworkmanager_impl.h"


CurlNetworkManager::CurlNetworkManager(QObject *parent) : QObject(parent)
{
    curlNetworkManagerImpl_ = new CurlNetworkManagerImpl(nullptr);
    connect(curlNetworkManagerImpl_, &CurlNetworkManagerImpl::requestFinished, this, &CurlNetworkManager::onRequestFinished, Qt::QueuedConnection);
    connect(curlNetworkManagerImpl_, &CurlNetworkManagerImpl::requestProgress, this, &CurlNetworkManager::onRequestProgress, Qt::QueuedConnection);
    connect(curlNetworkManagerImpl_, &CurlNetworkManagerImpl::requestNewData, this, &CurlNetworkManager::onRequestNewData, Qt::QueuedConnection);
}

CurlNetworkManager::~CurlNetworkManager()
{
    // Delete the CurlReply children first.
    // This is important since they have access to curlNetworkManagerImpl_ in their destructor.
    const QList<CurlReply *> replies = findChildren<CurlReply *>();
    for (auto it : replies)
        delete it;
    delete curlNetworkManagerImpl_;
}

CurlReply *CurlNetworkManager::get(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    quint64 requestId = curlNetworkManagerImpl_->get(request, ips, proxySettings);
    CurlReply *curlReply = new CurlReply(this, requestId);
    activeRequests_[requestId] = curlReply;
    return curlReply;
}

CurlReply *CurlNetworkManager::post(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    quint64 requestId = curlNetworkManagerImpl_->post(request, data, ips, proxySettings);
    CurlReply *curlReply = new CurlReply(this, requestId);
    activeRequests_[requestId] = curlReply;
    return curlReply;
}

CurlReply *CurlNetworkManager::put(const NetworkRequest &request, const QByteArray &data, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    quint64 requestId = curlNetworkManagerImpl_->put(request, data, ips, proxySettings);
    CurlReply *curlReply = new CurlReply(this, requestId);
    activeRequests_[requestId] = curlReply;
    return curlReply;
}

CurlReply *CurlNetworkManager::deleteResource(const NetworkRequest &request, const QStringList &ips, const types::ProxySettings &proxySettings /*= types::ProxySettings()*/)
{
    quint64 requestId = curlNetworkManagerImpl_->deleteResource(request, ips, proxySettings);
    CurlReply *curlReply = new CurlReply(this, requestId);
    activeRequests_[requestId] = curlReply;
    return curlReply;
}

void CurlNetworkManager::abort(CurlReply *reply)
{
    activeRequests_.remove(reply->id());
    curlNetworkManagerImpl_->abort(reply->id());
}

void CurlNetworkManager::onRequestFinished(quint64 requestId, CURLcode curlErrorCode)
{
    auto it = activeRequests_.find(requestId);
    if (it != activeRequests_.end()) {
        it.value()->setCurlErrorCode(curlErrorCode);
        emit it.value()->finished();
    }
}

void CurlNetworkManager::onRequestProgress(quint64 requestId, qint64 bytesReceived, qint64 bytesTotal)
{
    auto it = activeRequests_.find(requestId);
    if (it != activeRequests_.end())
        emit it.value()->progress(bytesReceived, bytesTotal);
}

void CurlNetworkManager::onRequestNewData(quint64 requestId, const QByteArray &newData)
{
    auto it = activeRequests_.find(requestId);
    if (it != activeRequests_.end()) {
        it.value()->appendNewData(newData);
        emit it.value()->readyRead();
    }
}

