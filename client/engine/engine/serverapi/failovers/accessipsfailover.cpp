#include "accessipsfailover.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QUrlQuery>

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "engine/serverapi/requests/accessipsrequest.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"

namespace server_api {

void AccessIpsFailover::getHostnames(bool bIgnoreSslErrors)
{
    SAFE_DELETE(connectStateWatcher_);
    connectStateWatcher_ = new ConnectStateWatcher(this, connectStateController_);

    AcessIpsRequest *request = new AcessIpsRequest(this);
    WS_ASSERT(request->requestType() == RequestType::kGet);
    NetworkRequest networkRequest(request->url(ip_).toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors);
    NetworkReply *reply = networkAccessManager_->get(networkRequest);

    QPointer<AcessIpsRequest> pointerToRequest(request);
    reply->setProperty("pointerToRequest", QVariant::fromValue(pointerToRequest));
    reply->setProperty("bIgnoreSslErrors", bIgnoreSslErrors);
    connect(reply, &NetworkReply::finished, this, &AccessIpsFailover::onNetworkRequestFinished);
}

QString AccessIpsFailover::name() const
{
    // the domain name has been reduced to 3 characters for log security
    return "acc: " + ip_.left(3);
}

void AccessIpsFailover::onNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    QPointer<AcessIpsRequest> pointerToRequest = reply->property("pointerToRequest").value<QPointer<AcessIpsRequest> >();
    bool isIgnoreSslErrors = reply->property("bIgnoreSslErrors").toBool();
    if (pointerToRequest) {
        if (!reply->isSuccess()) {
            // if connect state changed the retrying the request
            if (connectStateWatcher_->isVpnConnectStateChanged()) {
                this->getHostnames(isIgnoreSslErrors);
            } else {
                if (reply->error() ==  NetworkReply::NetworkError::SslError && !isIgnoreSslErrors)
                    emit finished(FailoverRetCode::kSslError, QStringList());
                else
                    emit finished(FailoverRetCode::kFailed, QStringList());
            }
        }
        else {
            pointerToRequest->handle(reply->readAll());
            if (pointerToRequest->retCode() == SERVER_RETURN_SUCCESS)
                emit finished(FailoverRetCode::kSuccess, pointerToRequest->hosts());
            else
                emit finished(FailoverRetCode::kFailed, QStringList());
        }
        pointerToRequest->deleteLater();
    }
}

} // namespace server_api

