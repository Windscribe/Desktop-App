#include "echfailover.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"

namespace failover {

void EchFailover::getData(bool bIgnoreSslErrors)
{
    QUrl url(urlString_);
    QUrlQuery query;
    query.addQueryItem("name", configDomainName_);
    query.addQueryItem("type", "TXT");
    url.setQuery(query);

    NetworkRequest networkRequest(url, 5000, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors);
    networkRequest.setContentTypeHeader("accept: application/dns-json");
    NetworkReply *reply = networkAccessManager_->get(networkRequest);
    connect(reply, &NetworkReply::finished, [=]() {
        if (!reply->isSuccess()) {
            emit finished(QVector<FailoverData>());
        } else {
            QVector<FailoverData> data = parseDataFromJson(reply->readAll());
            data = Utils::randomizeList<QVector<FailoverData> >(data);
            emit finished(data);
        }
        reply->deleteLater();
    });
}

QString EchFailover::name() const
{
    QUrl url(urlString_);
    // the domain name has been reduced for log security
    return "ech: " + url.host().left(3);
}

// returns an empty string if parsing failed
QVector<FailoverData> EchFailover::parseDataFromJson(const QByteArray &arr)
{
    QJsonParseError errCode;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
    if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        return QVector<FailoverData>();

    QJsonObject jsonObject = doc.object();
    if (!jsonObject.contains("Status") || jsonObject["Status"].toInt(1) != 0)
        return QVector<FailoverData>();

    if (!jsonObject.contains("Answer") || !jsonObject["Answer"].isArray())
        return QVector<FailoverData>();

    QJsonArray jsonArray = jsonObject["Answer"].toArray();
    WS_ASSERT(jsonArray.size() > 0);
    if (jsonArray.isEmpty())
        return QVector<FailoverData>();

    QVector<FailoverData> ret;
    for (auto obj : jsonArray) {
        QJsonObject jsonAnswer = obj.toObject();
        if (!jsonAnswer.contains("data") || !jsonAnswer.contains("TTL"))
            return QVector<FailoverData>();
        QString echConfig = jsonAnswer["data"].toString().remove("\"");
        int ttl = jsonAnswer["TTL"].toInt();
        if (ttl == 0)
            return QVector<FailoverData>();

        ret << FailoverData(domainName_, echConfig, ttl);
    }
    return ret;
}

} // namespace failover

