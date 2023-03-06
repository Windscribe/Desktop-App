#include "requestexecuterviafailover.h"

#include <QUrl>
#include <QUrlQuery>

#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/extraconfig.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

namespace server_api {

RequestExecuterViaFailover::RequestExecuterViaFailover(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    connectStateController_(connectStateController), networkAccessManager_(networkAccessManager), failover_(nullptr)
{
}

void RequestExecuterViaFailover::execute(QPointer<BaseRequest> request, failover::BaseFailover *failover, bool bIgnoreSslErrors)
{
    WS_ASSERT(request_ == nullptr);
    WS_ASSERT(failover_ == nullptr);
    request_ = request;
    failover_ = failover;
    bIgnoreSslErrors_ = bIgnoreSslErrors;
    connectStateWatcher_.reset(new ConnectStateWatcher(this, connectStateController_));

    connect(failover_, &failover::BaseFailover::finished, this, &RequestExecuterViaFailover::onFailoverFinished);
    failover_->getData(bIgnoreSslErrors_);
}

failover::FailoverData RequestExecuterViaFailover::failoverData() const
{
    WS_ASSERT(curIndFailoverData_ >= 0 && curIndFailoverData_ < failoverData_.count());
    return failoverData_[curIndFailoverData_];
}

void RequestExecuterViaFailover::onFailoverFinished(const QVector<failover::FailoverData> &data)
{
    // if the request has already been deleted before completion
    if (!request_) {
        emit finished(RequestExecuterRetCode::kRequestDeleted);
        return;
    }
    // if connect state changed then we can't be sure what failover worked right. Must repeat the request in ServerAPI
    if (connectStateWatcher_->isVpnConnectStateChanged()) {
        emit finished(RequestExecuterRetCode::kConnectStateChanged);
        return;
    }
    if (data.isEmpty()) {
        emit finished(RequestExecuterRetCode::kFailoverFailed);
        return;
    }

    WS_ASSERT(data.count() > 0);
    failoverData_ = data;
    curIndFailoverData_ = 0;
    executeBaseRequest(failoverData_[curIndFailoverData_]);
}

void RequestExecuterViaFailover::onNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    // if the request has already been deleted before completion
    if (!request_) {
        emit finished(RequestExecuterRetCode::kRequestDeleted);
        return;
    }

    // if connect state changed then we can't be sure what failover worked right. Must repeat the request in ServerAPI
    if (connectStateWatcher_->isVpnConnectStateChanged()) {
        emit finished(RequestExecuterRetCode::kConnectStateChanged);
        return;
    }

    if (!reply->isSuccess()) {
        // failover can contain several domains, let's try another one if there is one
        curIndFailoverData_++;
        if (curIndFailoverData_ >= failoverData_.count())
            emit finished(RequestExecuterRetCode::kFailoverFailed);
        else
            executeBaseRequest(failoverData_[curIndFailoverData_]);
        return;
    }

    QByteArray serverResponse = reply->readAll();
    if (ExtraConfig::instance().getLogAPIResponse()) {
        qCDebug(LOG_SERVER_API) << request_->name();
        qCDebugMultiline(LOG_SERVER_API) << serverResponse;
    }

    request_->handle(serverResponse);

    if (request_->networkRetCode() == SERVER_RETURN_INCORRECT_JSON) {
        // failover can contain several domains, let's try another one if there is one
        curIndFailoverData_++;
        if (curIndFailoverData_ >= failoverData_.count())
            emit finished(RequestExecuterRetCode::kFailoverFailed);
        else
            executeBaseRequest(failoverData_[curIndFailoverData_]);
        return;
    }

    emit finished(RequestExecuterRetCode::kSuccess);
}

void RequestExecuterViaFailover::executeBaseRequest(const failover::FailoverData &failoverData)
{
    // Make sure the network return code is reset if we've failed over
    request_->setNetworkRetCode(SERVER_RETURN_SUCCESS);

    NetworkRequest networkRequest(request_->url(failoverData.domain()).toString(), request_->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_);
    if (failover_->isEch()) {
        networkRequest.setEchConfig(failoverData.echConfig());
    }

    NetworkReply *reply;
    switch (request_->requestType()) {
        case RequestType::kGet:
            reply = networkAccessManager_->get(networkRequest);
            break;
        case RequestType::kPost:
            reply = networkAccessManager_->post(networkRequest, request_->postData());
            break;
        case RequestType::kDelete:
            reply = networkAccessManager_->deleteResource(networkRequest);
            break;
        case RequestType::kPut:
            reply = networkAccessManager_->put(networkRequest, request_->postData());
            break;
        default:
            WS_ASSERT(false);
    }
    connect(reply, &NetworkReply::finished, this, &RequestExecuterViaFailover::onNetworkRequestFinished);
}

QDebug operator<<(QDebug dbg, const RequestExecuterRetCode &f)
{
    QDebugStateSaver saver(dbg);
    switch (f) {
    case RequestExecuterRetCode::kSuccess:
        dbg << "kSuccess";
        break;
    case RequestExecuterRetCode::kRequestDeleted:
        dbg << "kRequestDeleted";
        break;
    case RequestExecuterRetCode::kFailoverFailed:
        dbg << "kFailoverFailed";
        break;
    case RequestExecuterRetCode::kConnectStateChanged:
        dbg << "kConnectStateChanged";
        break;
    default:
        WS_ASSERT(false);
    }

    return dbg;
}


} // namespace server_api
