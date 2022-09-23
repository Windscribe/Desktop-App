#include "pingtestrequest.h"

#include <QJsonDocument>

#include "utils/hardcodedsettings.h"

namespace server_api {

PingTestRequest::PingTestRequest(QObject *parent, int timeout) : BaseRequest(parent, RequestType::kGet, HardcodedSettings::instance().serverTunnelTestUrl(), true, timeout)
{
}

QUrl PingTestRequest::url() const
{
    QUrl url("https://" + hostname_);
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
