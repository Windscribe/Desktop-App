#include "serverconfigsrequest.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

#include "engine/openvpnversioncontroller.h"
#include "utils/logger.h"

namespace server_api {

ServerConfigsRequest::ServerConfigsRequest(QObject *parent, const QString &hostname, const QString &authHash) :
    BaseRequest(parent, RequestType::kGet, hostname),
    authHash_(authHash)
{
}

QUrl ServerConfigsRequest::url() const
{
    QUrl url("https://" + hostname_ + "/ServerConfigs");
    QUrlQuery query;

    query.addQueryItem("ovpn_version",
                       OpenVpnVersionController::instance().getSelectedOpenVpnVersion());
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString ServerConfigsRequest::name() const
{
    return "ServerConfigs";
}

void ServerConfigsRequest::handle(const QByteArray &arr)
{
    qCDebug(LOG_SERVER_API) << "API request ServerConfigs successfully executed";
    ovpnConfig_ = QByteArray::fromBase64(arr);
    setRetCode(SERVER_RETURN_SUCCESS);
}


} // namespace server_api {
