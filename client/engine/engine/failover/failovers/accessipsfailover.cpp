#include "accessipsfailover.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "engine/utils/urlquery_utils.h"
#include "utils/logger.h"

namespace failover {

void AccessIpsFailover::getData(bool bIgnoreSslErrors)
{
    QUrl url("https://" + ip_ + "/ApiAccessIps");
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query);
    urlquery_utils::addPlatformQueryItems(query);
    url.setQuery(query);

    NetworkRequest networkRequest(url.toString(), kTimeout, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors);
    NetworkReply *reply = networkAccessManager_->get(networkRequest);

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

    if (!reply->isSuccess()) {
        emit finished(QVector<FailoverData>());
    }
    else {
        QStringList hosts = handleRequest(reply->readAll());
        QVector<FailoverData> data;
        for (const QString &s : hosts)
            data << FailoverData(s);

        emit finished(data);
    }
}

QStringList AccessIpsFailover::handleRequest(const QByteArray &arr)
{
    QStringList hosts;
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json";
        return QStringList();
    }
    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("data")) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (data field not found)";
        return QStringList();
    }
    QJsonObject jsonData =  jsonObject["data"].toObject();
    if (!jsonData.contains("hosts")) {
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (hosts field not found)";
        return QStringList();
    }

    const QJsonArray jsonArray = jsonData["hosts"].toArray();
    for (const QJsonValue &value : jsonArray)
        hosts << value.toString();

    qCDebug(LOG_SERVER_API) << "API request ApiAccessIps successfully executed";
    return hosts;
}

} // namespace failover

