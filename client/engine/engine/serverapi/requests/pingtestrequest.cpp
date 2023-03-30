#include "pingtestrequest.h"

#include <QJsonDocument>

#include "engine/utils/urlquery_utils.h"

namespace server_api {

PingTestRequest::PingTestRequest(QObject *parent, int timeout) : BaseRequest(parent, RequestType::kGet, true, timeout)
{
}

QUrl PingTestRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kTunnelTest));
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query);
    urlquery_utils::addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString PingTestRequest::name() const
{
    return "PingTest";
}

void PingTestRequest::handle(const QByteArray &arr)
{
    data_ = arr;
}

} // namespace server_api {
