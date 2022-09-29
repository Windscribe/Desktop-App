#include "serverapi.h"

#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

#include "requests/loginrequest.h"
#include "requests/sessionrequest.h"
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
#include "requests/syncrobertrequest.h"

#ifdef Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace server_api {

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    bIsRequestsEnabled_(false),
    bIgnoreSslErrors_(false),
    failoverDisconnectedModeState_(FailoverState::kInProgress),
    failoverConnectedModeState_(FailoverState::kInProgress)
{
    failoverConnectedMode_ = new Failover(this, networkAccessManager);
    failoverDisconnectedMode_ = new Failover(this, networkAccessManager);
}

ServerAPI::~ServerAPI()
{
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
    return hostname_;
}

BaseRequest *ServerAPI::login(const QString &username, const QString &password, const QString &code2fa)
{
    LoginRequest *request = new LoginRequest(this, username, password, code2fa);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::session(const QString &authHash)
{
    SessionRequest *request = new SessionRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverLocations(const QString &language, const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList)
{
    ServerListRequest *request = new ServerListRequest(this, language, revision, isPro, protocol, alcList, connectStateController_);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverCredentials(const QString &authHash, PROTOCOL protocol)
{
    ServerCredentialsRequest *request = new ServerCredentialsRequest(this, authHash, protocol);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::deleteSession(const QString &authHash)
{
    DeleteSessionRequest *request = new DeleteSessionRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::serverConfigs(const QString &authHash)
{
    ServerConfigsRequest *request = new ServerConfigsRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::portMap(const QString &authHash)
{
    PortMapRequest *request = new PortMapRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::recordInstall()
{
    RecordInstallRequest *request = new RecordInstallRequest(this);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::confirmEmail(const QString &authHash)
{
    ConfirmEmailRequest *request = new ConfirmEmailRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::webSession(const QString authHash, WEB_SESSION_PURPOSE purpose)
{
    WebSessionRequest *request = new WebSessionRequest(this, authHash, purpose);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::myIP(int timeout)
{
    MyIpRequest *request = new MyIpRequest(this, timeout);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::checkUpdate(UPDATE_CHANNEL updateChannel)
{
    CheckUpdateRequest *request = new CheckUpdateRequest(this, updateChannel);

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

    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::debugLog(const QString &username, const QString &strLog)
{
    DebugLogRequest *request = new DebugLogRequest(this, username, strLog);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating)
{
    SpeedRatingRequest *request = new SpeedRatingRequest(this, authHash, speedRatingHostname, ip, rating);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::staticIps(const QString &authHash, const QString &deviceId)
{
    StaticIpsRequest *request = new StaticIpsRequest(this, authHash, deviceId);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::pingTest(uint timeout, bool bWriteLog)
{
    if (bWriteLog)
        qCDebug(LOG_SERVER_API) << "Do ping test with timeout: " << timeout;

    PingTestRequest *request = new PingTestRequest(this, timeout);
    if (!bWriteLog) request->setNotWriteToLog();
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::notifications(const QString &authHash)
{
    NotificationsRequest *request = new NotificationsRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::wgConfigsInit(const QString &authHash, const QString &clientPublicKey, bool deleteOldestKey)
{
    WgConfigsInitRequest *request = new WgConfigsInitRequest(this, authHash, clientPublicKey, deleteOldestKey);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::wgConfigsConnect(const QString &authHash,
                                 const QString &clientPublicKey, const QString &serverName, const QString &deviceId)
{
    WgConfigsConnectRequest *request = new WgConfigsConnectRequest(this, authHash, clientPublicKey, serverName, deviceId);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::getRobertFilters(const QString &authHash)
{
    GetRobertFiltersRequest *request = new GetRobertFiltersRequest(this, authHash);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::setRobertFilter(const QString &authHash, const types::RobertFilter &filter)
{
    SetRobertFiltersRequest *request = new SetRobertFiltersRequest(this, authHash, filter);
    executeRequest(request);
    return request;
}

BaseRequest *ServerAPI::syncRobert(const QString &authHash)
{
    SyncRobertRequest *request = new SyncRobertRequest(this, authHash);
    executeRequest(request);
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
                qCDebug(LOG_SERVER_API) << "API request " + pointerToRequest->name() + " failed:" << reply->errorString();
            emit pointerToRequest->finished();
        }
        else {
            pointerToRequest->handle(reply->readAll());
            emit pointerToRequest->finished();
        }
    }
}

void ServerAPI::executeRequest(BaseRequest *request)
{
    /*if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        QTimer::singleShot(0, this, [request] () {
            qCDebug(LOG_SERVER_API) << "API request " + request->name() + " failed: API not ready";
            request->setRetCode(SERVER_RETURN_API_NOT_READY);
            emit request->finished();
        });
        return;
    }*/
    //FIXME: getCurrentDnsServers() move to NetworkAccessManager
    NetworkRequest networkRequest(request->url(hostname_).toString(), request->timeout(), true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_);
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
