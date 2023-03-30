#include "speedratingrequest.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

SpeedRatingRequest::SpeedRatingRequest(QObject *parent, const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating) :
    BaseRequest(parent, RequestType::kPost),
    authHash_(authHash),
    speedRatingHostname_(speedRatingHostname),
    ip_(ip),
    rating_(rating)
{
}

QString SpeedRatingRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray SpeedRatingRequest::postData() const
{
    QUrlQuery postData;
    postData.addQueryItem("hostname", speedRatingHostname_);
    postData.addQueryItem("rating", QString::number(rating_));
    postData.addQueryItem("ip", ip_);
    urlquery_utils::addAuthQueryItems(postData, authHash_);
    urlquery_utils::addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl SpeedRatingRequest::url(const QString &domain) const
{
    return QUrl("https://" + hostname(domain, SudomainType::kApi) + "/SpeedRating");
}

QString SpeedRatingRequest::name() const
{
    return "SpeedRating";
}

void SpeedRatingRequest::handle(const QByteArray &arr)
{
    qCDebug(LOG_SERVER_API) << "SpeedRating request successfully executed";
    qCDebugMultiline(LOG_SERVER_API) << arr;
}

} // namespace server_api {
