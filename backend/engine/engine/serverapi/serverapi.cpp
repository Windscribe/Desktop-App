#include "serverapi.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSslSocket>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "engine/hardcodedsettings.h"
#include "engine/openvpnversioncontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "version/appversion.h"
#include "../tests/sessionandlocations_test.h"

const int typeIdWireGuardConfig = qRegisterMetaType<QSharedPointer<WireGuardConfig> >("QSharedPointer<WireGuardConfig>");

class ServerAPI::BaseRequest
{
public:
    enum class HandlerType { NONE, DNS, CURL };
    enum { DEFAULT_DNS_CACHING_TIMEOUT = -1, NO_DNS_CACHING = 0 };

    BaseRequest(const QString &hostname, int replyType, uint timeout, uint userRole)
        : isActive_(true), replyType_(replyType), timeout_(timeout), userRole_(userRole),
          startTime_(QDateTime::currentMSecsSinceEpoch()), hostname_(hostname),
          curlRequest_(nullptr), isCurlRequestSubmitted_(false), handlerType_(HandlerType::NONE) {}
    virtual ~BaseRequest() {
        if (!isCurlRequestSubmitted_)
            delete curlRequest_;
    }
    virtual int getDnsCachingTimeout() const { return DEFAULT_DNS_CACHING_TIMEOUT; }

    void setActive(bool value) { isActive_ = value; }
    void setCurlRequestSubmitted(bool value) { isCurlRequestSubmitted_ = value; }
    void setWaitingHandlerType(HandlerType type) { handlerType_ = type; }

    bool isActive() const { return isActive_; }
    bool isCurlRequestSubmitted() const { return isCurlRequestSubmitted_; }
    bool isWaitingForDnsResponse() const { return handlerType_ == HandlerType::DNS; }
    bool isWaitingForCurlResponse() const { return handlerType_ == HandlerType::CURL; }
    int getReplyType() const { return replyType_; }
    uint getTimeout() const { return timeout_; }
    uint getUserRole() const { return userRole_; }
    qint64 getStartTime() const { return startTime_; }
    const QString &getHostname() const { return hostname_; }

    CurlRequest *getCurlRequest() const { return curlRequest_; }
    CurlRequest *createCurlRequest() {
        Q_ASSERT(!curlRequest_);
        curlRequest_ = new CurlRequest;
        curlRequest_->setTimeout(timeout_);
        return curlRequest_;
    }

private:
    bool isActive_;
    int replyType_;
    uint timeout_;
    uint userRole_;
    qint64 startTime_;
    QString hostname_;
    CurlRequest *curlRequest_;
    bool isCurlRequestSubmitted_;
    HandlerType handlerType_;
};

namespace
{
// Interval, in ms, between polling requests, to remove expired and timed out.
const int REQUEST_POLL_INTERVAL_MS = 100;

QString GetPlatformName()
{
#ifdef Q_OS_WIN
    return "windows";
#elif defined Q_OS_MAC
    return "osx";
#else
    return "";
#endif
}

QUrlQuery MakeQuery(const QString &authHash, bool bAddOpenVpnVersion = false)
{
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrlQuery query;
    if (!authHash.isEmpty())
        query.addQueryItem("session_auth_hash", authHash);
    query.addQueryItem("time", strTimestamp);
    query.addQueryItem("client_auth_hash", md5Hash);
    if (bAddOpenVpnVersion)
        query.addQueryItem("ovpn_version",
                           OpenVpnVersionController::instance().getSelectedOpenVpnVersion());
    query.addQueryItem("platform", GetPlatformName());

    return query;
}

// ServerAPI request types.
class GenericRequest : public ServerAPI::BaseRequest
{
public:
    using ServerAPI::BaseRequest::BaseRequest;
};

class LoginRequest : public ServerAPI::BaseRequest
{
public:
    LoginRequest(const QString &username, const QString &password, const QString &code2fa,
                 const QString &hostname, int replyType, uint timeout, uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
          username_(username), password_(password), code2fa_(code2fa) {}

    const QString &getUsername() const { return username_; }
    const QString &getPassword() const { return password_; }
    const QString &getCode2FA() const { return code2fa_; }

private:
    QString username_;
    QString password_;
    QString code2fa_;
};

class AuthenticatedRequest : public ServerAPI::BaseRequest
{
public:
    AuthenticatedRequest(const QString &authhash, const QString &hostname,
                         int replyType, uint timeout, uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
        authhash_(authhash) {}

    const QString &getAuthHash() const { return authhash_; }

private:
    QString authhash_;
};

class ServerLocationsRequest : public AuthenticatedRequest
{
public:
    ServerLocationsRequest(const QString &authhash, const QString &language,
                           const QString &revision, bool isPro,
                           ProtocolType protocol, QStringList alcList, const QString &hostname,
                           int replyType, uint timeout, uint userRole)
        : AuthenticatedRequest(authhash, hostname, replyType, timeout, userRole),
          language_(language), revision_(revision), isPro_(isPro),
          protocol_(protocol), alcList_(alcList) {}

    const QString &getLanguage() const { return language_; }
    const QString &getRevision() const { return revision_; }
    bool getIsPro() const { return isPro_; }
    ProtocolType getProtocol() const { return protocol_; }
    const QStringList &getAlcList() const { return alcList_; }

private:
    QString language_;
    QString revision_;
    bool isPro_;
    ProtocolType protocol_;
    QStringList alcList_;
};

class ServerCredentialsRequest : public AuthenticatedRequest
{
public:
    ServerCredentialsRequest(const QString &authhash, ProtocolType protocol,
                             const QString &hostname, int replyType, uint timeout, uint userRole)
        : AuthenticatedRequest(authhash, hostname, replyType, timeout, userRole),
          protocol_(protocol) {}

    ProtocolType getProtocol() const { return protocol_; }

private:
    ProtocolType protocol_;
};

class CheckUpdateRequest : public ServerAPI::BaseRequest
{
public:
    CheckUpdateRequest(ProtoTypes::UpdateChannel updateChannel, const QString &hostname, int replyType, uint timeout,
                       uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
          updateChannel_(updateChannel) {}

    ProtoTypes::UpdateChannel getUpdateChannel() const { return updateChannel_; }

private:
    ProtoTypes::UpdateChannel updateChannel_;
};

class DebugLogRequest : public ServerAPI::BaseRequest
{
public:
    DebugLogRequest(const QString &username, const QString &logStr, const QString &hostname,
                    int replyType, uint timeout, uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
          username_(username), logStr_(logStr) {}

    const QString &getUsername() const { return username_; }
    const QString &getLogStr() const { return logStr_; }

private:
    QString username_;
    QString logStr_;
};

class GetMyIpRequest : public ServerAPI::BaseRequest
{
public:
    GetMyIpRequest(bool isDisconnected, const QString &hostname, int replyType, uint timeout,
                   uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
        isDisconnected_(isDisconnected) {}

    bool getIsDisconnected() const { return isDisconnected_; }

private:
    bool isDisconnected_;
};

class SpeedRatingRequest : public AuthenticatedRequest
{
public:
    SpeedRatingRequest(const QString &authhash, const QString &speedRatingHostname,
                       const QString &ip, int rating, const QString &hostname, int replyType,
                       uint timeout, uint userRole)
        : AuthenticatedRequest(authhash, hostname, replyType, timeout, userRole),
          speedRatingHostname_(speedRatingHostname), ip_(ip), rating_(rating) {}

    const QString &getSpeedRatingHostname() const { return speedRatingHostname_; }
    const QString &getIp() const { return ip_; }
    int getRating() const { return rating_; }

private:
    QString speedRatingHostname_;
    QString ip_;
    int rating_;
};

class StaticIpsRequest : public AuthenticatedRequest
{
public:
    StaticIpsRequest(const QString &authhash, const QString &deviceId, const QString &hostname,
                     int replyType, uint timeout, uint userRole)
        : AuthenticatedRequest(authhash, hostname, replyType, timeout, userRole),
          deviceId_(deviceId) {}

    const QString &getDeviceId() const { return deviceId_; }

private:
    QString deviceId_;
};

class PingRequest : public ServerAPI::BaseRequest
{
public:
    PingRequest(quint64 commandId, const QString &hostname, int replyType, uint timeout,
                uint userRole)
        : ServerAPI::BaseRequest(hostname, replyType, timeout, userRole),
          commandId_(commandId) {}
    int getDnsCachingTimeout() const override { return NO_DNS_CACHING; }

    quint64 getCommandId() const { return commandId_; }

private:
    quint64 commandId_;
};
} // namespace

ServerAPI::ServerAPI(QObject *parent, INetworkStateManager *networkStateManager) : QObject(parent),
    networkStateManager_(networkStateManager),
    bIsOnline_(true),
    hostMode_(HOST_MODE_HOSTNAME),
    bIsRequestsEnabled_(false),
    curUserRole_(0),
    bIgnoreSslErrors_(false),
    handleDnsResolveFuncTable_(),
    handleCurlReplyFuncTable_(),
    requestTimer_(this)
{
    connect(&curlNetworkManager_, SIGNAL(finished(CurlRequest*)),
            SLOT(onCurlNetworkRequestFinished(CurlRequest*)));
    connect(networkStateManager_, SIGNAL(stateChanged(bool, QString)),
            SLOT(onNetworkAccessibleChanged(bool)));

    dnsCache_ = new DnsCache(this);
    connect(dnsCache_, SIGNAL(resolved(bool,void*,QStringList)),
            SLOT(onDnsResolved(bool,void*,QStringList)));
    connect(dnsCache_, SIGNAL(ipsInCachChanged(QStringList)),
            SIGNAL(hostIpsChanged(QStringList)), Qt::DirectConnection);

    if (QSslSocket::supportsSsl())
    {
        qCDebug(LOG_SERVER_API) << "SSL version:" << QSslSocket::sslLibraryVersionString();
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "Fatal: SSL not supported";
    }

    handleDnsResolveFuncTable_[REPLY_LOGIN] = &ServerAPI::handleLoginDnsResolve;
    handleDnsResolveFuncTable_[REPLY_SESSION] = &ServerAPI::handleSessionDnsResolve;
    handleDnsResolveFuncTable_[REPLY_SERVER_LOCATIONS] = &ServerAPI::handleServerLocationsDnsResolve;
    handleDnsResolveFuncTable_[REPLY_SERVER_CREDENTIALS] = &ServerAPI::handleServerCredentialsDnsResolve;
    handleDnsResolveFuncTable_[REPLY_DELETE_SESSION] = &ServerAPI::handleDeleteSessionDnsResolve;
    handleDnsResolveFuncTable_[REPLY_SERVER_CONFIGS] = &ServerAPI::handleServerConfigsDnsResolve;
    handleDnsResolveFuncTable_[REPLY_PORT_MAP] = &ServerAPI::handlePortMapDnsResolve;
    handleDnsResolveFuncTable_[REPLY_MY_IP] = &ServerAPI::handleMyIPDnsResolve;
    handleDnsResolveFuncTable_[REPLY_CHECK_UPDATE] = &ServerAPI::handleCheckUpdateDnsResolve;
    handleDnsResolveFuncTable_[REPLY_RECORD_INSTALL] = &ServerAPI::handleRecordInstallDnsResolve;
    handleDnsResolveFuncTable_[REPLY_DEBUG_LOG] = &ServerAPI::handleDebugLogDnsResolve;
    handleDnsResolveFuncTable_[REPLY_SPEED_RATING] = &ServerAPI::handleSpeedRatingDnsResolve;
    handleDnsResolveFuncTable_[REPLY_PING_TEST] = &ServerAPI::handlePingTestDnsResolve;
    handleDnsResolveFuncTable_[REPLY_NOTIFICATIONS] = &ServerAPI::handleNotificationsDnsResolve;
    handleDnsResolveFuncTable_[REPLY_STATIC_IPS] = &ServerAPI::handleStaticIpsDnsResolve;
    handleDnsResolveFuncTable_[REPLY_CONFIRM_EMAIL] = &ServerAPI::handleConfirmEmailDnsResolve;
    handleDnsResolveFuncTable_[REPLY_WIREGUARD_CONFIG] = &ServerAPI::handleWireGuardConfigDnsResolve;

    handleCurlReplyFuncTable_[REPLY_ACCESS_IPS] = &ServerAPI::handleAccessIpsCurl;
    handleCurlReplyFuncTable_[REPLY_LOGIN] = &ServerAPI::handleSessionReplyCurl;
    handleCurlReplyFuncTable_[REPLY_SESSION] = &ServerAPI::handleSessionReplyCurl;
    handleCurlReplyFuncTable_[REPLY_SERVER_LOCATIONS] = &ServerAPI::handleServerLocationsCurl;
    handleCurlReplyFuncTable_[REPLY_SERVER_CREDENTIALS] = &ServerAPI::handleServerCredentialsCurl;
    handleCurlReplyFuncTable_[REPLY_DELETE_SESSION] = &ServerAPI::handleDeleteSessionCurl;
    handleCurlReplyFuncTable_[REPLY_SERVER_CONFIGS] = &ServerAPI::handleServerConfigsCurl;
    handleCurlReplyFuncTable_[REPLY_PORT_MAP] = &ServerAPI::handlePortMapCurl;
    handleCurlReplyFuncTable_[REPLY_MY_IP] = &ServerAPI::handleMyIPCurl;
    handleCurlReplyFuncTable_[REPLY_CHECK_UPDATE] = &ServerAPI::handleCheckUpdateCurl;
    handleCurlReplyFuncTable_[REPLY_RECORD_INSTALL] = &ServerAPI::handleRecordInstallCurl;
    handleCurlReplyFuncTable_[REPLY_DEBUG_LOG] = &ServerAPI::handleDebugLogCurl;
    handleCurlReplyFuncTable_[REPLY_SPEED_RATING] = &ServerAPI::handleSpeedRatingCurl;
    handleCurlReplyFuncTable_[REPLY_PING_TEST] = &ServerAPI::handlePingTestCurl;
    handleCurlReplyFuncTable_[REPLY_NOTIFICATIONS] = &ServerAPI::handleNotificationsCurl;
    handleCurlReplyFuncTable_[REPLY_STATIC_IPS] = &ServerAPI::handleStaticIpsCurl;
    handleCurlReplyFuncTable_[REPLY_CONFIRM_EMAIL] = &ServerAPI::handleConfirmEmailCurl;
    handleCurlReplyFuncTable_[REPLY_WIREGUARD_CONFIG] = &ServerAPI::handleWireGuardConfigCurl;

    connect(&requestTimer_, SIGNAL(timeout()), SLOT(onRequestTimer()));
    requestTimer_.start(REQUEST_POLL_INTERVAL_MS);
}

ServerAPI::~ServerAPI()
{
    delete dnsCache_;

    for (auto *rd : activeRequests_)
        delete rd;
    activeRequests_.clear();
}

uint ServerAPI::getAvailableUserRole()
{
    uint userRole = curUserRole_;
    curUserRole_++;
    return userRole;
}

void ServerAPI::setProxySettings(const ProxySettings &proxySettings)
{
    curlNetworkManager_.setProxySettings(proxySettings);
}

void ServerAPI::disableProxy()
{
    curlNetworkManager_.setProxyEnabled(false);
}

void ServerAPI::enableProxy()
{
    curlNetworkManager_.setProxyEnabled(true);
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
    hostMode_ = HOST_MODE_HOSTNAME;
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

void ServerAPI::submitDnsRequest(BaseRequest *request, const QString &forceHostname)
{
    if (request && request->isActive()) {
        request->setWaitingHandlerType(BaseRequest::HandlerType::DNS);
        if (forceHostname.isEmpty())
        {
            dnsCache_->resolve(hostname_, request->getDnsCachingTimeout(), request);
        }
        else
        {
            dnsCache_->resolve(forceHostname, request->getDnsCachingTimeout(), request);
        }
    }
}

void ServerAPI::submitCurlRequest(BaseRequest *request, CurlRequest::MethodType type,
                                  const QString &contentTypeHeader, const QString &hostname,
                                  const QStringList &ips)
{
    if (!request || !request->isActive())
        return;

    auto *curl_request = request->getCurlRequest();
    Q_ASSERT(curl_request != nullptr);

    // Make sure that cURL callback will be called on timeout.
    request->setWaitingHandlerType(BaseRequest::HandlerType::CURL);

    const auto current_time = QDateTime::currentMSecsSinceEpoch();
    const auto delay = static_cast<uint>(current_time - request->getStartTime());
    if (delay >= request->getTimeout())
        return;

    curlToRequestMap_[curl_request] = request;
    request->setCurlRequestSubmitted(true);

    const auto curl_timeout = request->getTimeout() - delay;

    switch (type) {
    case CurlRequest::METHOD_GET:
        curlNetworkManager_.get(curl_request, curl_timeout, hostname, ips);
        break;
    case CurlRequest::METHOD_POST:
        curlNetworkManager_.post(curl_request, curl_timeout, contentTypeHeader, hostname, ips);
        break;
    case CurlRequest::METHOD_PUT:
        curlNetworkManager_.put(curl_request, curl_timeout, contentTypeHeader, hostname, ips);
        break;
    case CurlRequest::METHOD_DELETE:
        curlNetworkManager_.deleteResource(curl_request, curl_timeout, hostname, ips);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void ServerAPI::onRequestTimer()
{
    const auto current_time = QDateTime::currentMSecsSinceEpoch();

    // Remove inactive and timed out requests.
    QMutableLinkedListIterator<BaseRequest*> it(activeRequests_);
    while (it.hasNext()) {
        auto *rd = it.next();
        bool inactive = false;
        bool timeout = false;
        if (!rd->isActive()) {
            inactive = true;
        } else if (current_time > rd->getStartTime() + rd->getTimeout()) {
            inactive = true;
            timeout = true;
        }
        if (inactive) {
            const auto *curlRequest = rd->getCurlRequest();
            if (curlRequest)
                curlToRequestMap_.remove(curlRequest);
            if (timeout) {
                // Callbacks are expected for timed out requests, but not cancelled/done ones.
                handleRequestTimeout(rd);
            }
            delete rd;
            it.remove();
        }
    }
}

// works with direct IP
void ServerAPI::accessIps(const QString &hostIp, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit accessIpsAnswer(SERVER_RETURN_API_NOT_READY, QStringList(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit accessIpsAnswer(SERVER_RETURN_NETWORK_ERROR, QStringList(), userRole);
        return;
    }

    QUrl url("https://" + hostIp + "/ApiAccessIps");
    url.setQuery(MakeQuery(""));

    auto *rd =
        createRequest<GenericRequest>(QString(), REPLY_ACCESS_IPS, NETWORK_TIMEOUT, userRole);
    if (rd) {
        auto *curl_request = rd->createCurlRequest();
        curl_request->setGetData(url.toString());
        submitCurlRequest(rd, CurlRequest::METHOD_GET, QString(), hostIp, QStringList());
    }
}

void ServerAPI::login(const QString &username, const QString &password, const QString &code2fa,
                      uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit loginAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::SessionStatus(), "", userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit loginAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(), "", userRole);
        return;
    }

    submitDnsRequest( createRequest<LoginRequest>(
        username, password, code2fa, hostname_, REPLY_LOGIN, NETWORK_TIMEOUT, userRole) );
 }

void ServerAPI::session(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit sessionAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::SessionStatus(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(), userRole);
        return;
    }

    submitDnsRequest( createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_SESSION, NETWORK_TIMEOUT, userRole) );
}

void ServerAPI::serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                                const QString &revision, bool isPro, ProtocolType protocol, const QStringList &alcList)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit serverLocationsAnswer(SERVER_RETURN_API_NOT_READY, QVector<apiinfo::Location>(), QStringList(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit serverLocationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<apiinfo::Location>(), QStringList(), userRole);
        return;
    }

    QString hostname;
    // if this is IP, then use without changes
    if (IpValidation::instance().isIp(hostname_))
    {
        hostname = hostname_;
    }
    // otherwise use domain assets.windscribe.com
    else
    {
        QString modifiedHostname = hostname_;
        if (modifiedHostname.startsWith("api", Qt::CaseInsensitive))
        {
            modifiedHostname = modifiedHostname.remove(0, 3);
            modifiedHostname.insert(0, "assets");
        }
        else
        {
            Q_ASSERT(false);
        }
        hostname = modifiedHostname;
    }

    submitDnsRequest(createRequest<ServerLocationsRequest>(
        authHash, language, revision, isPro, protocol,
        std::move(alcList), hostname, REPLY_SERVER_LOCATIONS, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::serverCredentials(const QString &authHash, uint userRole, ProtocolType protocol, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit serverCredentialsAnswer(SERVER_RETURN_API_NOT_READY, "", "", protocol, userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit serverCredentialsAnswer(SERVER_RETURN_NETWORK_ERROR, "", "", protocol, userRole);
        return;
    }

    submitDnsRequest(createRequest<ServerCredentialsRequest>(
        authHash, protocol, hostname_, REPLY_SERVER_CREDENTIALS, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::deleteSession(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed: API not ready";
        return;
    }

    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed: no network connectivity";
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_DELETE_SESSION, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::serverConfigs(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit serverConfigsAnswer(SERVER_RETURN_API_NOT_READY, QByteArray(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit serverConfigsAnswer(SERVER_RETURN_NETWORK_ERROR, QByteArray(), userRole);
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_SERVER_CONFIGS, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::portMap(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit portMapAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::PortMap(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit portMapAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::PortMap(), userRole);
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_PORT_MAP, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::recordInstall(uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        qCDebug(LOG_SERVER_API) << "API request RecordInstall failed: API not ready";
        return;
    }

    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "API request RecordInstall failed: no network connectivity";
        return;
    }

    submitDnsRequest(createRequest<GenericRequest>(
        hostname_, REPLY_RECORD_INSTALL, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::confirmEmail(uint userRole, const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        qCDebug(LOG_SERVER_API) << "API request Users failed: API not ready";
        return;
    }

    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "API request Users failed: no network connectivity";
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_CONFIRM_EMAIL, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::myIP(bool isDisconnected, uint userRole, bool isNeedCheckRequestsEnabled)
{
    // if previous myIp request not finished, mark that it should be ignored in handlers
    for (auto *rd : activeRequests_) {
        if (rd->getReplyType() == REPLY_MY_IP)
            rd->setActive(false);
    }

    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        //qCDebug(LOG_SERVER_API) << "API request MyIp failed: API not ready";
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    }

    submitDnsRequest(createRequest<GetMyIpRequest>(
        isDisconnected, hostname_, REPLY_MY_IP, GET_MY_IP_TIMEOUT, userRole));
}

void ServerAPI::checkUpdate(const ProtoTypes::UpdateChannel updateChannel, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        qCDebug(LOG_SERVER_API) << "Check update failed: API not ready";
        emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, true, userRole);
        return;
    }

    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "Check update failed: no network connectivity";
        emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, true, userRole);
        return;
    }

    submitDnsRequest(createRequest<CheckUpdateRequest>(
        updateChannel, hostname_, REPLY_CHECK_UPDATE, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::debugLog(const QString &username, const QString &strLog, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit debugLogAnswer(SERVER_RETURN_API_NOT_READY, userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit debugLogAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
        return;
    }

    submitDnsRequest(createRequest<DebugLogRequest>(
        username, strLog, hostname_, REPLY_DEBUG_LOG, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed: API not ready";
        return;
    }

    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed: no network connectivity";
        return;
    }

    submitDnsRequest(createRequest<SpeedRatingRequest>(authHash, speedRatingHostname, ip, rating,
        hostname_, REPLY_SPEED_RATING, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::staticIps(const QString &authHash, const QString &deviceId, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit staticIpsAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::StaticIps(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit staticIpsAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::StaticIps(), userRole);
        return;
    }

    submitDnsRequest(createRequest<StaticIpsRequest>(
        authHash, deviceId, hostname_, REPLY_STATIC_IPS, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::pingTest(quint64 cmdId, uint timeout)
{
    if (!bIsOnline_)
    {
        qCDebug(LOG_SERVER_API) << "PingTest request failed: no network connectivity";
        emit pingTestAnswer(SERVER_RETURN_NETWORK_ERROR, "");
        return;
    }

    const QString kCheckIpHostname = HardcodedSettings::instance().serverTunnelTestUrl();

    qCDebug(LOG_SERVER_API) << "Do ping test to:" << kCheckIpHostname << " with timeout: "
                            << timeout;

    submitDnsRequest(createRequest<PingRequest>(
        cmdId, kCheckIpHostname, REPLY_PING_TEST, timeout, 0), kCheckIpHostname);
}

void ServerAPI::cancelPingTest(quint64 cmdId)
{
    for (auto *rd : activeRequests_) {
        if (rd->getReplyType() != REPLY_PING_TEST)
            continue;
        auto *ping_rd = dynamic_cast<PingRequest*>(rd);
        if (ping_rd && ping_rd->getCommandId() == cmdId)
            rd->setActive(false);
    }
}

void ServerAPI::notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit notificationsAnswer(SERVER_RETURN_API_NOT_READY, QVector<apiinfo::Notification>(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit notificationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<apiinfo::Notification>(), userRole);
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_NOTIFICATIONS, NETWORK_TIMEOUT, userRole));
}

void ServerAPI::getWireGuardConfig(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_)
    {
        emit getWireGuardConfigAnswer(SERVER_RETURN_API_NOT_READY,
            QSharedPointer<WireGuardConfig>(), userRole);
        return;
    }

    if (!bIsOnline_)
    {
        emit getWireGuardConfigAnswer(SERVER_RETURN_NETWORK_ERROR,
            QSharedPointer<WireGuardConfig>(), userRole);
        return;
    }

    submitDnsRequest(createRequest<AuthenticatedRequest>(
        authHash, hostname_, REPLY_WIREGUARD_CONFIG, NETWORK_TIMEOUT, userRole));
}

bool ServerAPI::isOnline()
{
    bIsOnline_ = networkStateManager_->isOnline();
    return bIsOnline_;
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
    curlNetworkManager_.setIgnoreSslErrors(bIgnore);
}

void ServerAPI::onDnsResolved(bool success, void *userData, const QStringList &ips)
{
    // Make sure the request is pending. This will also handle the case when the request was deleted
    // by the polling thread, so that |userData| became invalid.
    auto *rd = static_cast<BaseRequest*>(userData);
    if (!activeRequests_.contains(rd) || !rd->isActive())
        return;

    const auto reply_type = rd->getReplyType();
    Q_ASSERT(reply_type >= 0 && reply_type < NUM_REPLY_TYPES);

    // If this request is active, call the corresponding handler.
    Q_ASSERT(rd->isWaitingForDnsResponse());
    Q_ASSERT(handleDnsResolveFuncTable_[reply_type] != nullptr);
    if (handleDnsResolveFuncTable_[reply_type])
        (this->*handleDnsResolveFuncTable_[reply_type])(rd, success, ips);

    if (rd->isWaitingForDnsResponse())
        rd->setWaitingHandlerType(BaseRequest::HandlerType::NONE);

    // If there is no active curl request, we are done.
    if (!rd->isWaitingForCurlResponse())
        rd->setActive(false);
}

void ServerAPI::onCurlNetworkRequestFinished(CurlRequest *curlRequest)
{
    // Make sure the request is pending.
    auto *rd = curlToRequestMap_.take(curlRequest);
    Q_ASSERT(!rd || rd->isCurlRequestSubmitted());
    if (!rd || !rd->isActive()) {
        delete curlRequest;
        return;
    }

    const auto reply_type = rd->getReplyType();
    Q_ASSERT(reply_type >= 0 && reply_type < NUM_REPLY_TYPES);

     // If this request is active, call the corresponding handler.
    if (rd->isActive()) {
        Q_ASSERT(rd->isWaitingForCurlResponse());
        Q_ASSERT(handleCurlReplyFuncTable_[reply_type] != nullptr);
        if (handleCurlReplyFuncTable_[reply_type])
            (this->*handleCurlReplyFuncTable_[reply_type])(rd, true);
    }

    if (rd->isWaitingForCurlResponse())
        rd->setWaitingHandlerType(BaseRequest::HandlerType::NONE);

    // We are done with this request.
    rd->setCurlRequestSubmitted(false);
    rd->setActive(false);
}

void ServerAPI::onNetworkAccessibleChanged(bool isOnline)
{
    Q_UNUSED(isOnline);
    //bIsOnline_ = isOnline;
}

void ServerAPI::handleRequestTimeout(BaseRequest *rd)
{
    const auto reply_type = rd->getReplyType();
    Q_ASSERT(reply_type >= 0 && reply_type < NUM_REPLY_TYPES);

    if (rd->isWaitingForCurlResponse()) {
        Q_ASSERT(handleCurlReplyFuncTable_[reply_type] != nullptr);
        if (handleCurlReplyFuncTable_[reply_type])
            (this->*handleCurlReplyFuncTable_[reply_type])(rd, false);
    } else if (rd->isWaitingForDnsResponse()) {
        Q_ASSERT(handleDnsResolveFuncTable_[reply_type] != nullptr);
        if (handleDnsResolveFuncTable_[reply_type])
            (this->*handleDnsResolveFuncTable_[reply_type])(rd, false, QStringList());
    }
}

void ServerAPI::handleLoginDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<LoginRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request Login failed: DNS-resolution failed";
        emit loginAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(), "",
                         crd->getUserRole());
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/Session");

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QUrlQuery postData;
    postData.addQueryItem("username", QUrl::toPercentEncoding(crd->getUsername()));
    postData.addQueryItem("password", QUrl::toPercentEncoding(crd->getPassword()));
    if (!crd->getCode2FA().isEmpty())
        postData.addQueryItem("2fa_code", QUrl::toPercentEncoding(crd->getCode2FA()));
    postData.addQueryItem("session_type_id", "3");
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("platform", GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setPostData(postData.toString(QUrl::FullyEncoded).toUtf8());
    curl_request->setUrl(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_POST, "Content-type: text/html; charset=utf-8",
                      crd->getHostname(), ips);
}

void ServerAPI::handleSessionDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request Session failed: DNS-resolution failed";
        emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(),
                           crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QUrl url("https://" + crd->getHostname() + "/Session?session_type_id=3&time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + crd->getAuthHash()
             + "&platform=" + GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleServerLocationsDnsResolve(BaseRequest *rd, bool success,
                                                const QStringList &ips)
{
    auto *crd = dynamic_cast<ServerLocationsRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request ServerLocations failed: DNS-resolution failed";
        emit serverLocationsAnswer(SERVER_RETURN_NETWORK_ERROR,
                                   QVector<apiinfo::Location>(), QStringList(),
                                   crd->getUserRole());
        return;
    }

    // generate alc parameter, if not empty
    QString alcField;
    const auto &alcList = crd->getAlcList();
    if (!alcList.isEmpty())
    {
        for (int i = 0; i < alcList.count(); ++i)
        {
            alcField += alcList[i];
            if (i < (alcList.count() - 1))
            {
                alcField += ",";
            }
        }
    }

    // if this is IP, then use ServerLocations request
    QUrl url;
    if (IpValidation::instance().isIp(hostname_))
    {
        url = QUrl("https://" + crd->getHostname() + "/ServerLocations");
        QUrlQuery query = MakeQuery(crd->getAuthHash());

        if (crd->getProtocol().isIkev2Protocol())
        {
            query.addQueryItem("sl_session_type", "4");
            query.addQueryItem("sl_premium", crd->getIsPro() ? "1" : "0");
        }

        query.addQueryItem("browser", "mobike");
        query.addQueryItem("platform", GetPlatformName());

        // add alc parameter in query, if not empty
        if (!alcField.isEmpty())
        {
            query.addQueryItem("alc", alcField);
        }

        url.setQuery(query);
    }
    // this is domain name, then use domain assets.windscribe.com
    else
    {
        QString modifiedHostname = hostname_;
        if (modifiedHostname.startsWith("api", Qt::CaseInsensitive))
        {
            modifiedHostname = modifiedHostname.remove(0, 3);
            modifiedHostname.insert(0, "assets");
        }
        else
        {
            Q_ASSERT(false);
        }
        QString strIsPro = crd->getIsPro() ? "1" : "0";
        url = QUrl("https://" + modifiedHostname + "/serverlist/mob-v2/" + strIsPro + "/" + crd->getRevision());

        // add alc parameter in query, if not empty
        if (!alcField.isEmpty())
        {
            QUrlQuery query;
            query.addQueryItem("platform", GetPlatformName());
            query.addQueryItem("alc", alcField);
            url.setQuery(query);
        }
    }

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleServerCredentialsDnsResolve(BaseRequest *rd, bool success,
                                                  const QStringList &ips)
{
    auto *crd = dynamic_cast<ServerCredentialsRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials"
            << crd->getProtocol().toShortString() << "failed: DNS-resolution failed";
        emit serverCredentialsAnswer(SERVER_RETURN_NETWORK_ERROR, "", "", crd->getProtocol(),
                                     crd->getUserRole());
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/ServerCredentials");
    QUrlQuery query = MakeQuery(crd->getAuthHash());
    if (crd->getProtocol().isOpenVpnProtocol())
    {
        query.addQueryItem("type", "openvpn");
    }
    else if (crd->getProtocol().isIkev2Protocol())
    {
        query.addQueryItem("type", "ikev2");
    }
    else
    {
        Q_ASSERT(false);
    }
    url.setQuery(query);

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleDeleteSessionDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed: DNS-resolution failed";
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/Session");
    url.setQuery(MakeQuery(crd->getAuthHash()));

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_DELETE, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleServerConfigsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request ServerConfigs failed: DNS-resolution failed";
        emit serverConfigsAnswer(SERVER_RETURN_NETWORK_ERROR, QByteArray(), crd->getUserRole());
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/ServerConfigs");
    url.setQuery(MakeQuery(crd->getAuthHash(), true));

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handlePortMapDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request PortMap failed: DNS-resolution failed";
        emit portMapAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::PortMap(),
                           crd->getUserRole());
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/PortMap");
    QUrlQuery query = MakeQuery(crd->getAuthHash());
    query.addQueryItem("version", "5");
    url.setQuery(query);

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleRecordInstallDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<GenericRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request RecordInstall failed: DNS-resolution failed";
        return;
    }

#ifdef Q_OS_WIN
    QUrl url("https://" + crd->getHostname() + "/RecordInstall/app/win");
#elif defined Q_OS_MAC
    QUrl url("https://" + crd->getHostname() + "/RecordInstall/app/mac");
#endif
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QString str = "time=" + strTimestamp + "&client_auth_hash=" + md5Hash
                + "&platform=" + GetPlatformName();

    auto *curl_request = crd->createCurlRequest();
    curl_request->setPostData(str.toUtf8());
    curl_request->setUrl(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_POST, "Content-type: text/html; charset=utf-8",
                      crd->getHostname(), ips);
}

void ServerAPI::handleConfirmEmailDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request ConfirmEmail failed: DNS-resolution failed";
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/Users");
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrlQuery postData;
    postData.addQueryItem("resend_confirmation", "1");
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", crd->getAuthHash());
    postData.addQueryItem("platform", GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setPostData(postData.toString(QUrl::FullyEncoded).toUtf8());
    curl_request->setUrl(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_PUT, "Content-type: text/html; charset=utf-8",
                      crd->getHostname(), ips);
}

void ServerAPI::handleMyIPDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<GetMyIpRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        emit myIPAnswer("N/A", false, crd->getIsDisconnected(), crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + crd->getHostname() + "/MyIp?time=" + strTimestamp + "&client_auth_hash="
             + md5Hash + "&platform=" + GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleCheckUpdateDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<CheckUpdateRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request CheckUpdate failed: DNS-resolution failed";
        emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, true, crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + crd->getHostname() + "/CheckUpdate");
    QUrlQuery query;
    query.addQueryItem("time", strTimestamp);
    query.addQueryItem("client_auth_hash", md5Hash);
    query.addQueryItem("platform", GetPlatformName());

    query.addQueryItem("version", AppVersion::instance().version());
    query.addQueryItem("build", AppVersion::instance().build());
    if (crd->getUpdateChannel() == ProtoTypes::UPDATE_CHANNEL_BETA)
    {
        query.addQueryItem("beta", "1");
    }
    else if (crd->getUpdateChannel() == ProtoTypes::UPDATE_CHANNEL_GUINEA_PIG)
    {
        query.addQueryItem("beta", "2");
    }

    // add OS version and build
    QString osVersion, osBuild;
    Utils::getOSVersionAndBuild(osVersion, osBuild);
    if (!osVersion.isEmpty())
    {
        query.addQueryItem("os_version", osVersion);
    }
    if (!osBuild.isEmpty())
    {
        query.addQueryItem("os_build", osBuild);
    }

    url.setQuery(query);

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleDebugLogDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<DebugLogRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "API request DebugLog failed: DNS-resolution failed";
        emit debugLogAnswer(SERVER_RETURN_NETWORK_ERROR, crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + crd->getHostname() + "/Report/applog");

    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    QByteArray ba = crd->getLogStr().toUtf8();
    postData.addQueryItem("logfile", ba.toBase64());
    if (!crd->getUsername().isEmpty())
        postData.addQueryItem("username", crd->getUsername());
    postData.addQueryItem("platform", GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setPostData(postData.toString(QUrl::FullyEncoded).toUtf8());
    curl_request->setUrl(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_POST,
        "Content-type: application/x-www-form-urlencoded", crd->getHostname(), ips);
}

void ServerAPI::handleSpeedRatingDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<SpeedRatingRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed: DNS-resolution failed";
        return;
    }

    QUrl url("https://" + crd->getHostname() + "/SpeedRating");
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QString str = "time=" + strTimestamp + "&client_auth_hash=" + md5Hash + "&session_auth_hash="
                  + crd->getAuthHash() + "&hostname=" + crd->getSpeedRatingHostname() + "&rating="
                  + QString::number(crd->getRating()) + "&ip=" + crd->getIp()
                  + "&platform=" + GetPlatformName();

    auto *curl_request = crd->createCurlRequest();
    curl_request->setPostData(str.toUtf8());
    curl_request->setUrl(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_POST, "Content-type: text/html; charset=utf-8",
                      crd->getHostname(), ips);
}

void ServerAPI::handleNotificationsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "Notifications request failed: DNS-resolution failed";
        emit notificationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<apiinfo::Notification>(),
                                 crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + crd->getHostname() + "/Notifications?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + crd->getAuthHash()
             + "&platform=" + GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleWireGuardConfigDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<AuthenticatedRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "WgConfigs request failed: DNS-resolution failed";
        emit getWireGuardConfigAnswer(SERVER_RETURN_NETWORK_ERROR,
            QSharedPointer<WireGuardConfig>(), crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + crd->getHostname() + "/WgConfigs?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + crd->getAuthHash()
             + "&platform=" + GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleStaticIpsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<StaticIpsRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "StaticIps request failed: DNS-resolution failed";
        emit staticIpsAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::StaticIps(),
                             crd->getUserRole());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

#ifdef Q_OS_WIN
    QString strOs = "win";
#else
    QString strOs = "mac";
#endif

    qDebug() << "device_id for StaticIps request:" << crd->getDeviceId();
    QUrl url("https://" + crd->getHostname() + "/StaticIps?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + crd->getAuthHash()
             + "&device_id=" + crd->getDeviceId() + "&os=" + strOs
             + "&platform=" + GetPlatformName());

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handlePingTestDnsResolve(BaseRequest *rd, bool success, const QStringList &ips)
{
    auto *crd = dynamic_cast<PingRequest*>(rd);
    Q_ASSERT(crd);

    if (!success) {
        qCDebug(LOG_SERVER_API) << "PingTest request failed: DNS-resolution failed";
        emit pingTestAnswer(SERVER_RETURN_NETWORK_ERROR, "");
        return;
    }

    QString strUrl = QString("https://") + crd->getHostname();
    QUrl url(strUrl);

    auto *curl_request = crd->createCurlRequest();
    curl_request->setGetData(url.toString());
    submitCurlRequest(crd, CurlRequest::METHOD_GET, QString(), crd->getHostname(), ips);
}

void ServerAPI::handleAccessIpsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        SERVER_API_RET_CODE retCode;
        /*if (reply->error() == QNetworkReply::ProxyAuthenticationRequiredError)
        {
            retCode = SERVER_RETURN_PROXY_AUTH_FAILED;
        }
        else*/ if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            retCode = SERVER_RETURN_SSL_ERROR;
        }
        else
        {
            retCode = SERVER_RETURN_NETWORK_ERROR;
        }

        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit accessIpsAnswer(retCode, QStringList(), userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();
        qCDebug(LOG_SERVER_API) << arr;

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json";
            emit accessIpsAnswer(SERVER_RETURN_INCORRECT_JSON, QStringList(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (data field not found)";
            emit accessIpsAnswer(SERVER_RETURN_INCORRECT_JSON, QStringList(), userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("hosts"))
        {
            qCDebug(LOG_SERVER_API) << "API request ApiAccessIps incorrect json (hosts field not found)";
            emit accessIpsAnswer(SERVER_RETURN_INCORRECT_JSON, QStringList(), userRole);
            return;
        }

        QJsonArray jsonArray = jsonData["hosts"].toArray();

        QStringList listHosts;
        Q_FOREACH(const QJsonValue &value, jsonArray)
        {
            listHosts << value.toString();
        }
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps successfully executed";
        emit accessIpsAnswer(SERVER_RETURN_SUCCESS, listHosts, userRole);
    }
}

void ServerAPI::handleSessionReplyCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const int replyType = rd->getReplyType();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;
    if (curlRetCode != CURLE_OK)
    {
        SERVER_API_RET_CODE retCode;

        if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            retCode = SERVER_RETURN_SSL_ERROR;
        }
        else
        {
            retCode = SERVER_RETURN_NETWORK_ERROR;
        }

        if (replyType == REPLY_LOGIN)
        {
            qCDebug(LOG_SERVER_API) << "API request Login failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
            emit loginAnswer(retCode, apiinfo::SessionStatus(), "", userRole);
        }
        else
        {
            qCDebug(LOG_SERVER_API) << "API request Session failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
            emit sessionAnswer(retCode, apiinfo::SessionStatus(), userRole);
        }
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

#ifdef TEST_API_FROM_FILES
        arr = SessionAndLocationsTest::instance().getSessionData();
#endif

        qCDebug(LOG_SERVER_API) << arr;

#ifdef TEST_CREATE_API_FILES
        QFile file("c:\\5\\session.api");
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(arr);
        }
#endif

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            if (replyType == REPLY_LOGIN)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json";
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), "", userRole);
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json";
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), userRole);
            }
            return;
        }
        QJsonObject jsonObject = doc.object();

        apiinfo::SessionStatus sessionStatus;

        if (jsonObject.contains("errorCode"))
        {
            int errorCode = jsonObject["errorCode"].toInt();

            if (errorCode == 701 || errorCode == 702 || errorCode == 703 || errorCode == 706)
            {
                if (replyType == REPLY_LOGIN)
                {
                    qCDebug(LOG_SERVER_API) << "API request Login return bad username";
                    emit loginAnswer(SERVER_RETURN_BAD_USERNAME, apiinfo::SessionStatus(), "", userRole);
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << "API request Session return bad username";
                    emit sessionAnswer(SERVER_RETURN_BAD_USERNAME, apiinfo::SessionStatus(), userRole);
                }
            }
            else
            {
                if (replyType == REPLY_LOGIN)
                {
                    if (errorCode == 1340)
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return missing 2FA code";
                        emit loginAnswer(SERVER_RETURN_MISSING_CODE2FA, apiinfo::SessionStatus(), "", userRole);
                    }
                    else if (errorCode == 1341)
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return invalid 2FA code";
                        emit loginAnswer(SERVER_RETURN_BAD_CODE2FA, apiinfo::SessionStatus(), "", userRole);
                    }
                    else
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return error";
                        emit loginAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(), "", userRole);
                    }
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << "API request Session return error";
                    emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::SessionStatus(), userRole);
                }
            }
            return;
        }

        if (!jsonObject.contains("data"))
        {
            if (replyType == REPLY_LOGIN)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json (data field not found)";
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), "", userRole);
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json (data field not found)";
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), userRole);
            }
            return;
        }
        QString authHash;
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (jsonData.contains("session_auth_hash"))
        {
            authHash = jsonData["session_auth_hash"].toString();
        }

        QString outErrorMsg;
        success = sessionStatus.initFromJson(jsonData, outErrorMsg);
        if (!success)
        {
            if (replyType == REPLY_LOGIN)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json:" << outErrorMsg;
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), "", userRole);
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json:" << outErrorMsg;
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::SessionStatus(), userRole);
            }
        }

        if (replyType == REPLY_LOGIN)
        {
            qCDebug(LOG_SERVER_API) << "API request Login successfully executed";
            emit loginAnswer(SERVER_RETURN_SUCCESS, sessionStatus, authHash, userRole);
        }
        else
        {
            qCDebug(LOG_SERVER_API) << "API request Session successfully executed";
            emit sessionAnswer(SERVER_RETURN_SUCCESS, sessionStatus, userRole);
        }
    }
}

void ServerAPI::handleServerLocationsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "API request ServerLocations failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            emit serverLocationsAnswer(SERVER_RETURN_SSL_ERROR, QVector<apiinfo::Location>(), QStringList(), userRole);
        }
        else
        {
            emit serverLocationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<apiinfo::Location>(), QStringList(), userRole);
        }
    }
    else
    {
        const auto *crd = dynamic_cast<ServerLocationsRequest*>(rd);
        Q_ASSERT(crd);
        lastLocationsLanguage_ = crd->getLanguage();
        QByteArray arr = curlRequest->getAnswer();

#ifdef TEST_CREATE_API_FILES
        QFile file("c:\\5\\locations.api");
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(arr);
        }
#endif

#ifdef TEST_API_FROM_FILES
        arr = SessionAndLocationsTest::instance().getLocationsData();
#endif

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (!jsonObject.contains("info"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (info field not found)";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }

        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (data field not found)";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }
        // parse revision number
        QJsonObject jsonInfo = jsonObject["info"].toObject();
        bool isChanged = jsonInfo["changed"].toInt() != 0;
        int newRevision = jsonInfo["revision"].toInt();
        QString revisionHash = jsonInfo["revision_hash"].toString();

        if (isChanged)
        {
            qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision changed =" << newRevision
                                    << ", revision_hash =" << revisionHash;

            // parse locations array
            const QJsonArray jsonData = jsonObject["data"].toArray();

            QVector<apiinfo::Location> serverLocations;
            QStringList forceDisconnectNodes;

            for (const QJsonValue &value: jsonData)
            {
                QJsonObject obj = value.toObject();

                apiinfo::Location sl;
                if (sl.initFromJson(obj, forceDisconnectNodes))
                {
                    serverLocations << sl;
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << arr;
                    qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (data field not found)";
                    emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
                    return;
                }
            }
            emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, serverLocations, forceDisconnectNodes, userRole);
        }
        else
        {
            qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision not changed";
            emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, QVector<apiinfo::Location>(), QStringList(), userRole);
        }
    }
}

void ServerAPI::handleServerCredentialsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    const auto *crd = dynamic_cast<ServerCredentialsRequest*>(rd);
    Q_ASSERT(crd);

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            emit serverCredentialsAnswer(SERVER_RETURN_SSL_ERROR, "", "", crd->getProtocol(), userRole);
        }
        else
        {
            emit serverCredentialsAnswer(SERVER_RETURN_NETWORK_ERROR, "", "", crd->getProtocol(), userRole);
        }
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "incorrect json";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", crd->getProtocol(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode"))
        {
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "return error code:" << arr;
            emit serverCredentialsAnswer(SERVER_RETURN_SUCCESS, "", "", crd->getProtocol(), userRole);
            return;
        }

        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "incorrect json (data field not found)";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", crd->getProtocol(), userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("username") || !jsonData.contains("password"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "incorrect json (username/password fields not found)";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", crd->getProtocol(), userRole);
            return;
        }
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << crd->getProtocol().toShortString() << "successfully executed";
        emit serverCredentialsAnswer(SERVER_RETURN_SUCCESS,
                                     QByteArray::fromBase64(jsonData["username"].toString().toUtf8()),
                                     QByteArray::fromBase64(jsonData["password"].toString().toUtf8()), crd->getProtocol(), userRole);
    }
}

void ServerAPI::handleDeleteSessionCurl(BaseRequest *rd, bool success)
{
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession successfully executed";
    }
}

void ServerAPI::handleServerConfigsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "API request ServerConfigs failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            emit serverConfigsAnswer(SERVER_RETURN_SSL_ERROR, QByteArray(), userRole);
        }
        else
        {
            emit serverConfigsAnswer(SERVER_RETURN_NETWORK_ERROR, QByteArray(), userRole);
        }
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "API request ServerConfigs successfully executed";
        QByteArray ovpnConfig = QByteArray::fromBase64(curlRequest->getAnswer());
        emit serverConfigsAnswer(SERVER_RETURN_SUCCESS, ovpnConfig, userRole);
    }
}

void ServerAPI::handlePortMapCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "API request PortMap failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        if (curlNetworkManager_.isCurlSslError(curlRetCode) && !bIgnoreSslErrors_)
        {
            emit portMapAnswer(SERVER_RETURN_SSL_ERROR, apiinfo::PortMap(), userRole);
        }
        else
        {
            emit portMapAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::PortMap(), userRole);
        }
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::PortMap(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (data field not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::PortMap(), userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("portmap"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap field not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::PortMap(), userRole);
            return;
        }

        QJsonArray jsonArr = jsonData["portmap"].toArray();

        apiinfo::PortMap portMap;
        if (!portMap.initFromJson(jsonArr))
        {
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap required fields not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::PortMap(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "API request PortMap successfully executed";
        emit portMapAnswer(SERVER_RETURN_SUCCESS, portMap, userRole);
    }
}

void ServerAPI::handleMyIPCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    const auto *crd = dynamic_cast<GetMyIpRequest*>(rd);
    Q_ASSERT(crd);

    if (curlRetCode != CURLE_OK)
    {
        emit myIPAnswer("N/A", false, crd->getIsDisconnected(), userRole);
        return;
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json";
            emit myIPAnswer("N/A", false, crd->getIsDisconnected(), userRole);
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(data field not found)";
            emit myIPAnswer("N/A", false, crd->getIsDisconnected(), userRole);
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("user_ip"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(user_ip field not found)";
            emit myIPAnswer("N/A", false, crd->getIsDisconnected(), userRole);
        }
        emit myIPAnswer(jsonData["user_ip"].toString(), true, crd->getIsDisconnected(), userRole);
    }
}

void ServerAPI::handleCheckUpdateCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "Check update failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, true, userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode"))
        {
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }

        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("update_needed_flag"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }

        int updateNeeded = jsonData["update_needed_flag"].toInt();
        if (updateNeeded != 1)
        {
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }

        if (!jsonData.contains("latest_version"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }
        if (!jsonData.contains("update_url"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(false, "", ProtoTypes::UPDATE_CHANNEL_RELEASE, 0, "", true, false, userRole);
            return;
        }
        int supported = true;
        if (jsonData.contains("supported"))
        {
            supported = jsonData["supported"].toInt();
        }

        ProtoTypes::UpdateChannel updateChannel = ProtoTypes::UPDATE_CHANNEL_RELEASE;
        if (jsonData.contains("is_beta"))
        {
            updateChannel = static_cast<ProtoTypes::UpdateChannel>(jsonData["is_beta"].toInt());
        }

        int latestBuild = 0;
        if (jsonData.contains("latest_build"))
        {
            latestBuild = jsonData["latest_build"].toInt();
        }

        QString latestVersion = jsonData["latest_version"].toString();
        QString updateUrl = jsonData["update_url"].toString();

        emit checkUpdateAnswer(true, latestVersion, updateChannel, latestBuild, updateUrl, supported == 1, false, userRole);
    }
}

void ServerAPI::handleRecordInstallCurl(BaseRequest *rd, bool success)
{
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "RecordInstall request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "RecordInstall request successfully executed";
        QByteArray arr = curlRequest->getAnswer();
        qCDebug(LOG_SERVER_API) << arr;
    }
}

void ServerAPI::handleConfirmEmailCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "Users request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit confirmEmailAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();
        qCDebug(LOG_SERVER_API) << arr;
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("email_sent"))
        {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        int emailSent = jsonData["email_sent"].toInt();
        emit confirmEmailAnswer(emailSent == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleDebugLogCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "DebugLog request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit debugLogAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("success"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        int is_success = jsonData["success"].toInt();
        emit debugLogAnswer(is_success == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleSpeedRatingCurl(BaseRequest *rd, bool success)
{
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "SpeedRating request successfully executed";
        QByteArray arr = curlRequest->getAnswer();
        qCDebug(LOG_SERVER_API) << arr;
    }
}

void ServerAPI::handlePingTestCurl(BaseRequest *rd, bool success)
{
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "PingTest request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit pingTestAnswer(SERVER_RETURN_NETWORK_ERROR, "");
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();
        emit pingTestAnswer(SERVER_RETURN_SUCCESS, arr);
    }
}

void ServerAPI::handleNotificationsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "Notifications request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit notificationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<apiinfo::Notification>(), userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Notification>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Notification>(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        QJsonArray jsonNotifications = jsonData["notifications"].toArray();

        QVector<apiinfo::Notification> notifications;

        Q_FOREACH(const QJsonValue &value, jsonNotifications)
        {
            QJsonObject obj = value.toObject();

            apiinfo::Notification n;
            if (!n.initFromJson(obj))
            {
                qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications (not all required fields)";
                emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Notification>(), userRole);
                return;
            }

            notifications.push_back(n);
        }
        qCDebug(LOG_SERVER_API) << "Notifications request successfully executed";
        emit notificationsAnswer(SERVER_RETURN_SUCCESS, notifications, userRole);
    }
}

void ServerAPI::handleWireGuardConfigCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "WgConfigs request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit getWireGuardConfigAnswer(SERVER_RETURN_NETWORK_ERROR,
            QSharedPointer<WireGuardConfig>(), userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for WgConfigs";
            emit getWireGuardConfigAnswer(SERVER_RETURN_INCORRECT_JSON,
                QSharedPointer<WireGuardConfig>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for WgConfigs";
            emit getWireGuardConfigAnswer(SERVER_RETURN_INCORRECT_JSON,
                QSharedPointer<WireGuardConfig>(), userRole);
            return;
        }
        QJsonObject jsonData = jsonObject["data"].toObject();
        if (!jsonData.contains("success"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for WgConfigs";
            emit getWireGuardConfigAnswer(SERVER_RETURN_INCORRECT_JSON,
                QSharedPointer<WireGuardConfig>(), userRole);
            return;
        }

        int is_success = jsonData["success"].toInt();
        if (!is_success) {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Bad JSON for WgConfigs: success == 0";
            emit getWireGuardConfigAnswer(SERVER_RETURN_INCORRECT_JSON,
                QSharedPointer<WireGuardConfig>(), userRole);
            return;
        }

        QJsonObject jsonConfig = jsonData["config"].toObject();
        QSharedPointer<WireGuardConfig> userconfig(new WireGuardConfig());
        if (!userconfig->initFromJson(jsonConfig)) {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Incorrect JSON for WgConfigs";
            emit getWireGuardConfigAnswer(SERVER_RETURN_INCORRECT_JSON,
                QSharedPointer<WireGuardConfig>(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "WgConfigs request successfully executed";
        emit getWireGuardConfigAnswer(SERVER_RETURN_SUCCESS, userconfig, userRole);
    }
}

void ServerAPI::handleStaticIpsCurl(BaseRequest *rd, bool success)
{
    const int userRole = rd->getUserRole();
    const auto *curlRequest = rd->getCurlRequest();
    CURLcode curlRetCode = success ? curlRequest->getCurlRetCode() : CURLE_OPERATION_TIMEDOUT;

    if (curlRetCode != CURLE_OK)
    {
        qCDebug(LOG_SERVER_API) << "StaticIps request failed(" << curlRetCode << "):" << curl_easy_strerror(curlRetCode);
        emit staticIpsAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::StaticIps(), userRole);
    }
    else
    {
        QByteArray arr = curlRequest->getAnswer();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data"))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();

        apiinfo::StaticIps staticIps;
        if (!staticIps.initFromJson(jsonData))
        {
            qCDebug(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "StaticIps request successfully executed";
        emit staticIpsAnswer(SERVER_RETURN_SUCCESS, staticIps, userRole);
    }
}
