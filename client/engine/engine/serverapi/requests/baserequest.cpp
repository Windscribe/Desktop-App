#include "baserequest.h"
#include "version/appversion.h"
#include "utils/utils.h"

namespace server_api {

BaseRequest::BaseRequest(QObject *parent, RequestType requestType) : QObject(parent),
    requestType_(requestType)
{
}

BaseRequest::BaseRequest(QObject *parent, RequestType requestType, bool isUseFailover, int timeout): QObject(parent),
    requestType_(requestType),
    bUseFailover_(isUseFailover),
    timeout_(timeout)
{
}

QUrlQuery BaseRequest::urlQuery() const
{
    addQueryItems(urlQuery_);

    // add platform and app version
    urlQuery_.addQueryItem("platform", Utils::getPlatformNameSafe());
    urlQuery_.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    return urlQuery_;
}

QString BaseRequest::contentTypeHeader() const
{
    return QString();
}

QByteArray BaseRequest::postData() const
{
    return QByteArray();
}

void BaseRequest::addPlatformQueryItems(QUrlQuery &urlQuery) const
{

}

void BaseRequest::addAuthQueryItems(QUrlQuery &urlQuery, const QString &/*authHash = QString()*/) const
{

}

RequestType BaseRequest::requestType() const
{
    return requestType_;
}

void BaseRequest::setRetCode(SERVER_API_RET_CODE retCode)
{
    retCode_ = retCode;
}

SERVER_API_RET_CODE BaseRequest::retCode() const
{
    return retCode_;
}


} // namespace server_api {
