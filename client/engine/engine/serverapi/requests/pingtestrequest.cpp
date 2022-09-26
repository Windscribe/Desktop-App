#include "pingtestrequest.h"

#include <QJsonDocument>

namespace server_api {

PingTestRequest::PingTestRequest(QObject *parent, const QString &hostname, int timeout) : BaseRequest(parent, RequestType::kGet, hostname, true, timeout)
{
}

QUrl PingTestRequest::url() const
{
    QUrl url("https://" + hostname(SudomainType::kTunnelTest));
    QUrlQuery query;
    addAuthQueryItems(query);
    addPlatformQueryItems(query);
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
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
