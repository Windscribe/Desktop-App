#include "pinghost_curl.h"
#include <QJsonDocument>
#include "../connectstatecontroller/iconnectstatecontroller.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

PingHost_Curl::PingHost_Curl(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager) :
    QObject(parent), connectStateController_(stateController), networkAccessManager_(networkAccessManager)
{
}

PingHost_Curl::~PingHost_Curl()
{
    clearPings();
}

void PingHost_Curl::addHostForPing(const QString &id, const QString &ip, const QString &hostame)
{
    // is host already pinging?
    if (pingingHosts_.contains(id))
        return;

    QUrl url(hostame);
    NetworkRequest networkRequest(url.toString(), PING_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), false);
    networkRequest.setOverrideIp(ip);
    NetworkReply *reply = networkAccessManager_->get(networkRequest);
    connect(reply, &NetworkReply::finished, this, &PingHost_Curl::onNetworkRequestFinished);
    reply->setProperty("id", id);
    reply->setProperty("fromDisconnectedState", connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED);
    pingingHosts_[id] = reply;
}

void PingHost_Curl::clearPings()
{
    for (QMap<QString, NetworkReply *>::iterator it = pingingHosts_.begin(); it != pingingHosts_.end(); ++it)
        it.value()->deleteLater();

    pingingHosts_.clear();
}

void PingHost_Curl::setProxySettings(const types::ProxySettings &proxySettings)
{
    // nothing todo here, since proxy settings already taken into account in NetworkAccessManager
}

void PingHost_Curl::disableProxy()
{
    // nothing todo here
}

void PingHost_Curl::enableProxy()
{
    // nothing todo here
}

void PingHost_Curl::onNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);  // don't forget to delete
    QString id = reply->property("id").toString();
    bool bFromDisconnectedState = reply->property("fromDisconnectedState").toBool();

    auto it = pingingHosts_.find(id);
    if (it != pingingHosts_.end()) {

        if (reply->isSuccess()) {
            int timems = parseReplyString(reply->readAll());
            if (timems != -1)
                // Convert to milliseconds dividing by 1000
                emit pingFinished(true, round(timems / 1000.0), id, bFromDisconnectedState);
            else
                emit pingFinished(false, 0, id, bFromDisconnectedState);

        } else {
            emit pingFinished(false, 0, id, bFromDisconnectedState);
        }

        pingingHosts_.remove(id);
    }
}

int PingHost_Curl::parseReplyString(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        return -1;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("rtt")) {
        QString value = jsonObject["rtt"].toString();
        if (value.isEmpty()) {
            WS_ASSERT(false);
            return -1;
        }
        bool bOk;
        int ret = value.toInt(&bOk);
        if (!bOk) {
            WS_ASSERT(false);
            return -1;
        }
        return ret;
    }

    return -1;
}

