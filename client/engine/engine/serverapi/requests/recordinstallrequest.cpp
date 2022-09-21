#include "recordinstallrequest.h"

#include <QJsonDocument>

#include "utils/logger.h"

namespace server_api {

RecordInstallRequest::RecordInstallRequest(QObject *parent, const QString &hostname) :
    BaseRequest(parent, RequestType::kPost, hostname)
{
}

QString RecordInstallRequest::contentTypeHeader() const
{
    return "Content-type: text/html; charset=utf-8";
}

QByteArray RecordInstallRequest::postData() const
{
    QUrlQuery postData;
    addAuthQueryItems(postData);
    addPlatformQueryItems(postData);
    return postData.toString(QUrl::FullyEncoded).toUtf8();
}

QUrl RecordInstallRequest::url() const
{
#ifdef Q_OS_WIN
    QUrl url("https://" + hostname_ + "/RecordInstall/app/win");
#elif defined Q_OS_MAC
    QUrl url("https://" + hostname_ + "/RecordInstall/app/mac");
#elif defined Q_OS_LINUX
    QUrl url("https://" + hostname_ + "/RecordInstall/app/linux");
#endif
    return url;
}

QString RecordInstallRequest::name() const
{
    return "RecordInstall";
}

void RecordInstallRequest::handle(const QByteArray &arr)
{
    qCDebug(LOG_SERVER_API) << "RecordInstall request successfully executed";
    setRetCode(SERVER_RETURN_SUCCESS);
}

} // namespace server_api {
