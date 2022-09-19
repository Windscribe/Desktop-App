#include "authenticatedrequest.h"

#include <QCryptographicHash>

#include "utils/hardcodedsettings.h"
#include "utils/ws_assert.h"

namespace server_api {

AuthenticatedRequest::AuthenticatedRequest(QObject *parent, RequestType requestType, const QString &authHash) :
    BaseRequest(parent, requestType),
    authHash_(authHash)
{
}

AuthenticatedRequest::AuthenticatedRequest(QObject *parent, RequestType requestType, bool isUseFailover, int timeout, const QString &authHash) :
    BaseRequest(parent, requestType, isUseFailover, timeout),
    authHash_(authHash)
{
}

void AuthenticatedRequest::addQueryItems(QUrlQuery &urlQuery) const
{
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    WS_ASSERT(!authHash_.isEmpty());
    urlQuery.addQueryItem("session_auth_hash", authHash_);
    urlQuery.addQueryItem("time", strTimestamp);
    urlQuery.addQueryItem("client_auth_hash", md5Hash);
}


} // namespace server_api {
