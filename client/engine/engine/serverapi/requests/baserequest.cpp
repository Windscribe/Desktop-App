#include "baserequest.h"

#include <QCryptographicHash>

#include "utils/hardcodedsettings.h"
#include "utils/ws_assert.h"
#include "utils/ipvalidation.h"


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

QString BaseRequest::contentTypeHeader() const
{
    return QString();
}

QByteArray BaseRequest::postData() const
{
    return QByteArray();
}

QString BaseRequest::hostname(const QString &domain, SudomainType subdomain) const
{
    // if this is IP, return without change
    if (IpValidation::isIp(domain)) {
        if (subdomain == SudomainType::kAssets) {
            return domain + "/" + HardcodedSettings::instance().serverAssetsSubdomain();
        } else if (subdomain == SudomainType::kTunnelTest) {
            return domain + "/" + HardcodedSettings::instance().serverTunnelTestSubdomain();
        } else {
            return domain;
        }
    }

    if (subdomain == SudomainType::kApi)
        return HardcodedSettings::instance().serverApiSubdomain() + "." + domain;
    else if (subdomain == SudomainType::kAssets)
        return HardcodedSettings::instance().serverAssetsSubdomain() + "." + domain;
    else if (subdomain == SudomainType::kTunnelTest)
        return HardcodedSettings::instance().serverTunnelTestSubdomain() + "." + domain;

    WS_ASSERT(false);
    return "";
}

void BaseRequest::setCancelled(bool cancelled)
{
    cancelled_ = cancelled;
}

bool BaseRequest::isCancelled() const
{
    return cancelled_;
}

} // namespace server_api {
