#include "deletesessionrequest.h"

#include "utils/logger.h"

namespace server_api {

DeleteSessionRequest::DeleteSessionRequest(QObject *parent, const QString &authHash) : BaseRequest(parent, RequestType::kDelete),
    authHash_(authHash)
{
}

QUrl DeleteSessionRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Session");
    QUrlQuery query;
    addAuthQueryItems(query, authHash_);
    addPlatformQueryItems(query);
    url.setQuery(query);
    return url;
}

QString DeleteSessionRequest::name() const
{
    return "DeleteSession";
}

void DeleteSessionRequest::handle(const QByteArray &arr)
{
    // nothing todo here
    qCDebug(LOG_SERVER_API) << "API request DeleteSession successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
