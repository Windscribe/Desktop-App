#include "serverapi.h"

#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

#include "requests/loginrequest.h"
#include "requests/sessionrequest.h"
#include "requests/accessipsrequest.h"
#include "requests/serverlistrequest.h"
#include "requests/servercredentialsrequest.h"
#include "requests/deletesessionrequest.h"
#include "requests/serverconfigsrequest.h"
#include "requests/portmaprequest.h"
#include "requests/recordinstallrequest.h"
#include "requests/confirmemailrequest.h"
#include "requests/websessionrequest.h"
#include "requests/myiprequest.h"
#include "requests/checkupdaterequest.h"
#include "requests/debuglogrequest.h"
#include "requests/speedratingrequest.h"
#include "requests/staticipsrequest.h"
#include "requests/pingtestrequest.h"
#include "requests/notificationsrequest.h"
#include "requests/getrobertfiltersrequest.h"
#include "requests/setrobertfiltersrequest.h"
#include "requests/wgconfigsinitrequest.h"
#include "requests/wgconfigsconnectrequest.h"

#ifdef Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace server_api {

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    isProxyEnabled_(false),
    bIsRequestsEnabled_(false),
    bIgnoreSslErrors_(false)
{
}

ServerAPI::~ServerAPI()
{
}

void ServerAPI::setProxySettings(const types::ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

void ServerAPI::disableProxy()
{
    isProxyEnabled_ = false;
}

void ServerAPI::enableProxy()
{
    isProxyEnabled_ = true;
}

void ServerAPI::setRequestsEnabled(bool bEnable)
{
    qCDebug(LOG_SERVER_API) << "setRequestsEnabled:" << bEnable;
    bIsRequestsEnabled_ = bEnable;
}

bool ServerAPI::isRequestsEnabled() const
{
    return bIsRequestsEnabled_;
}

void ServerAPI::setHostname(const QString &hostname)
{
    qCDebug(LOG_SERVER_API) << "setHostname:" << hostname;
    hostname_ = hostname;
}

QString ServerAPI::getHostname() const
{
    // if this is IP, return without change
    if (IpValidation::instance().isIp(hostname_))
    {
        return hostname_;
    }
    // otherwise return hostname without "api." appendix
    else
    {
        QString modifiedHostname = hostname_;
        if (modifiedHostname.startsWith("api.", Qt::CaseInsensitive))
        {
            modifiedHostname = modifiedHostname.remove(0, 4);
        }
        return modifiedHostname;
    }
}

// works with direct IP
BaseRequest *ServerAPI::accessIps(const QString &hostIp)
{
    AcessIpsRequest *request = new AcessIpsRequest(this, hostIp);
    executeRequest(request, false);
    return request;
}

BaseRequest *ServerAPI::login(const QString &username, const QString &password, const QString &code2fa)
{
    LoginRequest *request = new LoginRequest(this, hostname_, username, password, code2fa);
    executeRequest(request, false);
    return request;
}

BaseRequest *ServerAPI::session(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    SessionRequest *request = new SessionRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverLocations(const QString &language, bool isNeedCheckRequestsEnabled,
                                const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList)
{
    ServerListRequest *request = new ServerListRequest(this, hostname_, language, revision, isPro, protocol, alcList, connectStateController_);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverCredentials(const QString &authHash, PROTOCOL protocol, bool isNeedCheckRequestsEnabled)
{
    ServerCredentialsRequest *request = new ServerCredentialsRequest(this, hostname_, authHash, protocol);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::deleteSession(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    DeleteSessionRequest *request = new DeleteSessionRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::serverConfigs(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    ServerConfigsRequest *request = new ServerConfigsRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::portMap(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    PortMapRequest *request = new PortMapRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::recordInstall(bool isNeedCheckRequestsEnabled)
{
    RecordInstallRequest *request = new RecordInstallRequest(this, hostname_);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::confirmEmail(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    ConfirmEmailRequest *request = new ConfirmEmailRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::webSession(const QString authHash, WEB_SESSION_PURPOSE purpose, bool isNeedCheckRequestsEnabled)
{
    WebSessionRequest *request = new WebSessionRequest(this, hostname_, authHash, purpose);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::myIP(int timeout, bool isNeedCheckRequestsEnabled)
{
    MyIpRequest *request = new MyIpRequest(this, hostname_, timeout);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::checkUpdate(UPDATE_CHANNEL updateChannel, bool isNeedCheckRequestsEnabled)
{
    CheckUpdateRequest *request = new CheckUpdateRequest(this, hostname_, updateChannel);

    // This check will only be useful in the case that we expand our supported linux OSes and the platform flag is not added for that OS
    if (Utils::getPlatformName().isEmpty()) {
        qCDebug(LOG_SERVER_API) << "Check update failed: platform name is empty";
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
            request->setRetCode(SERVER_RETURN_SUCCESS);
            emit request->finished();
        });
        return request;
    }

    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::debugLog(const QString &username, const QString &strLog, bool isNeedCheckRequestsEnabled)
{
    DebugLogRequest *request = new DebugLogRequest(this, hostname_, username, strLog);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, bool isNeedCheckRequestsEnabled)
{
    SpeedRatingRequest *request = new SpeedRatingRequest(this, hostname_, authHash, speedRatingHostname, ip, rating);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::staticIps(const QString &authHash, const QString &deviceId, bool isNeedCheckRequestsEnabled)
{
    StaticIpsRequest *request = new StaticIpsRequest(this, hostname_, authHash, deviceId);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::pingTest(uint timeout, bool bWriteLog)
{
    if (bWriteLog)
        qCDebug(LOG_SERVER_API) << "Do ping test to:" << HardcodedSettings::instance().serverTunnelTestUrl() << " with timeout: " << timeout;

    PingTestRequest *request = new PingTestRequest(this, timeout);
    if (!bWriteLog) request->setNotWriteToLog();
    executeRequest(request, false);
    return request;
}

BaseRequest *ServerAPI::notifications(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    NotificationsRequest *request = new NotificationsRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::wgConfigsInit(const QString &authHash, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey)
{
    WgConfigsInitRequest *request = new WgConfigsInitRequest(this, hostname_, authHash, clientPublicKey, deleteOldestKey);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::wgConfigsConnect(const QString &authHash, bool isNeedCheckRequestsEnabled,
                                 const QString &clientPublicKey, const QString &serverName, const QString &deviceId)
{
    WgConfigsConnectRequest *request = new WgConfigsConnectRequest(this, hostname_, authHash, clientPublicKey, serverName, deviceId);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::getRobertFilters(const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    GetRobertFiltersRequest *request = new GetRobertFiltersRequest(this, hostname_, authHash);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

BaseRequest *ServerAPI::setRobertFilter(const QString &authHash, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter)
{
    SetRobertFiltersRequest *request = new SetRobertFiltersRequest(this, hostname_, authHash, filter);
    executeRequest(request, isNeedCheckRequestsEnabled);
    return request;
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

void ServerAPI::handleNetworkRequestFinished()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    QPointer<BaseRequest> pointerToRequest = reply->property("pointerToRequest").value<QPointer<BaseRequest> >();
    if (pointerToRequest) {

        if (!reply->isSuccess()) {
            if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
                pointerToRequest->setRetCode(SERVER_RETURN_SSL_ERROR);
            else
                pointerToRequest->setRetCode(SERVER_RETURN_NETWORK_ERROR);

            if (pointerToRequest->isWriteToLog())
                qCDebug(LOG_SERVER_API) << "API request " + pointerToRequest->name() + " failed(" << reply->errorString() << ")";
            emit pointerToRequest->finished();
        }
        else {
            pointerToRequest->handle(reply->readAll());
            emit pointerToRequest->finished();
        }
    }
}

types::ProxySettings ServerAPI::currentProxySettings() const
{
    if (isProxyEnabled_) {
        return proxySettings_;
    } else {
        return types::ProxySettings();
    }
}

void ServerAPI::executeRequest(BaseRequest *request, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
            request->setRetCode(SERVER_RETURN_API_NOT_READY);
            emit request->finished();
        });
        return;
    }

    NetworkRequest networkRequest(request->url().toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply;
    switch (request->requestType()) {
        case RequestType::kGet:
            reply = networkAccessManager_->get(networkRequest);
            break;
        case RequestType::kPost:
            reply = networkAccessManager_->post(networkRequest, request->postData());
            break;
        case RequestType::kDelete:
            reply = networkAccessManager_->deleteResource(networkRequest);
            break;
        case RequestType::kPut:
            reply = networkAccessManager_->put(networkRequest, request->postData());
            break;
        default:
            WS_ASSERT(false);
    }

    connect(request, &BaseRequest::destroyed, reply, &NetworkReply::deleteLater);

    QPointer<BaseRequest> pointerToRequest(request);
    reply->setProperty("pointerToRequest",  QVariant::fromValue(pointerToRequest));
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleNetworkRequestFinished);
}

} // namespace server_api
