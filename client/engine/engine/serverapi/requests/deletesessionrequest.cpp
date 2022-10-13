#include "deletesessionrequest.h"

#include "utils/logger.h"
#include "engine/utils/urlquery_utils.h"

namespace server_api {

DeleteSessionRequest::DeleteSessionRequest(QObject *parent, const QString &authHash) : BaseRequest(parent, RequestType::kDelete),
    authHash_(authHash)
{
}

QUrl DeleteSessionRequest::url(const QString &domain) const
{
    QUrl url("https://" + hostname(domain, SudomainType::kApi) + "/Session");
    QUrlQuery query;
    urlquery_utils::addAuthQueryItems(query, authHash_);
    urlquery_utils::addPlatformQueryItems(query);
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
}

} // namespace server_api {
