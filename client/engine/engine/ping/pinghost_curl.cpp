#include "pinghost_curl.h"
#include <QJsonDocument>
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "utils/ws_assert.h"

PingHost_Curl::PingHost_Curl(QObject *parent, NetworkAccessManager *networkAccessManager, const QString &ip, const QString &hostname) :
    IPingHost(parent), networkAccessManager_(networkAccessManager), ip_(ip), hostname_(hostname)
{
}

void PingHost_Curl::ping()
{
    WS_ASSERT(!isAlreadyPinging_);

    QUrl url(hostname_);
    NetworkRequest networkRequest(url.toString(), PING_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), false);
    networkRequest.setOverrideIp(ip_);
    // We add all ips to the firewall exceptions at once in the EngineLocationsModel, so there is no need to do it NetworkAccessManager
    networkRequest.setIsWhiteListIps(false);
    NetworkReply *reply = networkAccessManager_->get(networkRequest);
    connect(reply, &NetworkReply::finished, this, &PingHost_Curl::onNetworkRequestFinished);
    isAlreadyPinging_ = true;
}


void PingHost_Curl::onNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);  // don't forget to delete

    if (reply->isSuccess()) {
        int timems = parseReplyString(reply->readAll());
        if (timems != -1)
            // Convert to milliseconds dividing by 1000
            emit pingFinished(true, round(timems / 1000.0), ip_);
        else
            emit pingFinished(false, 0, ip_);

    } else {
        emit pingFinished(false, 0, ip_);
    }

    isAlreadyPinging_ = false;
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

