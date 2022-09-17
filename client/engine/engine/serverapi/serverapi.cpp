#include "serverapi.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSslSocket>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

#include "utils/ws_assert.h"
#include "utils/hardcodedsettings.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "version/appversion.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"

#ifdef Q_OS_LINUX
    #include "utils/linuxutils.h"
#endif

namespace
{
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
    query.addQueryItem("platform", Utils::getPlatformNameSafe());
    query.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    return query;
}
} // namespace

ServerAPI::ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    connectStateController_(connectStateController),
    networkAccessManager_(networkAccessManager),
    isProxyEnabled_(false),
    hostMode_(HOST_MODE_HOSTNAME),
    bIsRequestsEnabled_(false),
    curUserRole_(0),
    bIgnoreSslErrors_(false),
    myIpReply_(nullptr)
{

    if (QSslSocket::supportsSsl())
    {
        qCDebug(LOG_SERVER_API) << "SSL version:" << QSslSocket::sslLibraryVersionString();
    }
    else
    {
        qCDebug(LOG_SERVER_API) << "Fatal: SSL not supported";
    }
}

ServerAPI::~ServerAPI()
{
}

uint ServerAPI::getAvailableUserRole()
{
    uint userRole = curUserRole_;
    curUserRole_++;
    return userRole;
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

// works with direct IP
void ServerAPI::accessIps(const QString &hostIp, uint userRole)
{
    QUrl url("https://" + hostIp + "/ApiAccessIps");
    url.setQuery(MakeQuery(""));

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleAccessIps);
}

void ServerAPI::login(const QString &username, const QString &password, const QString &code2fa,
                      uint userRole)
{
    QString url = "https://" + hostname_ + "/Session";

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QUrlQuery postData;
    postData.addQueryItem("username", QUrl::toPercentEncoding(username));
    postData.addQueryItem("password", QUrl::toPercentEncoding(password));
    if (!code2fa.isEmpty())
        postData.addQueryItem("2fa_code", QUrl::toPercentEncoding(code2fa));
    postData.addQueryItem("session_type_id", "3");
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    NetworkRequest request(url, NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");

    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    reply->setProperty("isLoginRequest", true);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSessionReply);
}

void ServerAPI::session(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit sessionAnswer(SERVER_RETURN_API_NOT_READY, types::SessionStatus(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QString url = "https://" + hostname_ + "/Session?session_type_id=3&time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString();

    NetworkRequest request(url, NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    reply->setProperty("isLoginRequest", false);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSessionReply);
}

void ServerAPI::serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                                const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit serverLocationsAnswer(SERVER_RETURN_API_NOT_READY, QVector<apiinfo::Location>(), QStringList(), userRole);
        return;
    }

    QString hostname;
    // if this is IP, then use without changes
    if (IpValidation::instance().isIp(hostname_)) {
        hostname = hostname_;
    } else  {   // otherwise use domain assets.windscribe.com
        QString modifiedHostname = hostname_;
        if (modifiedHostname.startsWith("api", Qt::CaseInsensitive)) {
            modifiedHostname = modifiedHostname.remove(0, 3);
            modifiedHostname.insert(0, "assets");
        } else {
            WS_ASSERT(false);
        }
        hostname = modifiedHostname;
    }

    // generate alc parameter, if not empty
    QString alcField;
    if (!alcList.isEmpty()) {
        for (int i = 0; i < alcList.count(); ++i) {
            alcField += alcList[i];
            if (i < (alcList.count() - 1)) {
                alcField += ",";
            }
        }
    }

    QSettings settings;
    QString countryOverride = settings.value("countryOverride", "").toString();

    QString modifiedHostname = hostname_;
    if (modifiedHostname.startsWith("api", Qt::CaseInsensitive)) {
        modifiedHostname = modifiedHostname.remove(0, 3);
        modifiedHostname.insert(0, "assets");
    } else if (IpValidation::instance().isIp(modifiedHostname)) {
        modifiedHostname += "/assets";
    } else {
        WS_ASSERT(false);
    }
    QString strIsPro = isPro ? "1" : "0";
    QUrl url = QUrl("https://" + modifiedHostname + "/serverlist/mob-v2/" + strIsPro + "/" + revision);

    // add alc parameter in query, if not empty
    QUrlQuery query;
    if (!alcField.isEmpty()) {
        query.addQueryItem("platform", Utils::getPlatformNameSafe());
        query.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
        query.addQueryItem("alc", alcField);
    }
    if (!countryOverride.isEmpty() && connectStateController_->currentState() != CONNECT_STATE::CONNECT_STATE_DISCONNECTED) {
        query.addQueryItem("country_override", countryOverride);
        qCDebug(LOG_SERVER_API) << "API request ServerLocations added countryOverride = " << countryOverride;
    }
    url.setQuery(query);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    reply->setProperty("isFromDisconnectedVPNState", connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleServerLocations);
}

void ServerAPI::serverCredentials(const QString &authHash, uint userRole, PROTOCOL protocol, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit serverCredentialsAnswer(SERVER_RETURN_API_NOT_READY, "", "", protocol, userRole);
        return;
    }

    QUrl url("https://" + hostname_ + "/ServerCredentials");
    QUrlQuery query = MakeQuery(authHash);
    if (protocol.isOpenVpnProtocol())
        query.addQueryItem("type", "openvpn");
    else if (protocol.isIkev2Protocol())
        query.addQueryItem("type", "ikev2");
    else
        WS_ASSERT(false);

    url.setQuery(query);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    reply->setProperty("protocol", protocol.toInt());
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleServerCredentials);
}

void ServerAPI::deleteSession(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed: API not ready";
        return;
    }

    QUrl url("https://" + hostname_ + "/Session");
    url.setQuery(MakeQuery(authHash));

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->deleteResource(request);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleDeleteSession);
}

void ServerAPI::serverConfigs(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit serverConfigsAnswer(SERVER_RETURN_API_NOT_READY, QByteArray(), userRole);
        return;
    }

    QUrl url("https://" + hostname_ + "/ServerConfigs");
    url.setQuery(MakeQuery(authHash, true));

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleServerConfigs);
}

void ServerAPI::portMap(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit portMapAnswer(SERVER_RETURN_API_NOT_READY, types::PortMap(), userRole);
        return;
    }

    QUrl url("https://" + hostname_ + "/PortMap");
    QUrlQuery query = MakeQuery(authHash);
    query.addQueryItem("version", "5");
    url.setQuery(query);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handlePortMap);
}

void ServerAPI::recordInstall(uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "API request RecordInstall failed: API not ready";
        return;
    }

#ifdef Q_OS_WIN
    QUrl url("https://" + hostname_ + "/RecordInstall/app/win");
#elif defined Q_OS_MAC
    QUrl url("https://" + crd->getHostname() + "/RecordInstall/app/mac");
#elif defined Q_OS_LINUX
    QUrl url("https://" + crd->getHostname() + "/RecordInstall/app/linux");
#endif
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QString str = "time=" + strTimestamp + "&client_auth_hash=" + md5Hash
                + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString();

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, str.toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleRecordInstall);
}

void ServerAPI::confirmEmail(uint userRole, const QString &authHash, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "API request Users failed: API not ready";
        return;
    }

    QUrl url("https://" + hostname_ + "/Users");
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrlQuery postData;
    postData.addQueryItem("resend_confirmation", "1");
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", authHash);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleConfirmEmail);
}

void ServerAPI::webSession(const QString authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "API request WebSession failed: API not ready";
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();
    QUrl url("https://" + hostname_ + "/WebSession");

    QUrlQuery postData;
    postData.addQueryItem("temp_session", "1");
    postData.addQueryItem("session_type_id", "1");
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("session_auth_hash", authHash);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleWebSession);
}

void ServerAPI::myIP(bool isDisconnected, uint userRole, bool isNeedCheckRequestsEnabled)
{
    // if previous myIp request not finished, mark that it should be ignored in handlers
    SAFE_DELETE_LATER(myIpReply_);

    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/MyIp?time=" + strTimestamp + "&client_auth_hash="
             + md5Hash + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), GET_MY_IP_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    myIpReply_ = networkAccessManager_->get(request);
    myIpReply_->setProperty("userRole", userRole);
    myIpReply_->setProperty("isDisconnected", isDisconnected);
    connect(myIpReply_, &NetworkReply::finished, this, &ServerAPI::handleMyIP);
}

void ServerAPI::checkUpdate(UPDATE_CHANNEL updateChannel, uint userRole, bool isNeedCheckRequestsEnabled)
{
    // This check will only be useful in the case that we expand our supported linux OSes and the platform flag is not added for that OS
    if (Utils::getPlatformName().isEmpty()) {
        qCDebug(LOG_SERVER_API) << "Check update failed: platform name is empty";
        emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
        return;
    }

    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "Check update failed: API not ready";
        emit checkUpdateAnswer(types::CheckUpdate(), true, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/CheckUpdate");
    QUrlQuery query;
    query.addQueryItem("time", strTimestamp);
    query.addQueryItem("client_auth_hash", md5Hash);
    query.addQueryItem("platform", Utils::getPlatformNameSafe());
    query.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
    query.addQueryItem("version", AppVersion::instance().version());
    query.addQueryItem("build", AppVersion::instance().build());

    if (updateChannel == UPDATE_CHANNEL_BETA)
        query.addQueryItem("beta", "1");
    else if (updateChannel == UPDATE_CHANNEL_GUINEA_PIG)
        query.addQueryItem("beta", "2");
    else if (updateChannel == UPDATE_CHANNEL_INTERNAL)
        query.addQueryItem("beta", "3");

    // add OS version and build
    QString osVersion, osBuild;
    Utils::getOSVersionAndBuild(osVersion, osBuild);

    if (!osVersion.isEmpty())
        query.addQueryItem("os_version", osVersion);

    if (!osBuild.isEmpty())
        query.addQueryItem("os_build", osBuild);

    url.setQuery(query);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleCheckUpdate);
}

void ServerAPI::debugLog(const QString &username, const QString &strLog, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit debugLogAnswer(SERVER_RETURN_API_NOT_READY, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Report/applog");

    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    QByteArray ba = strLog.toUtf8();
    postData.addQueryItem("logfile", ba.toBase64());
    if (!username.isEmpty())
        postData.addQueryItem("username", username);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: application/x-www-form-urlencoded");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleDebugLog);
}

void ServerAPI::speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed: API not ready";
        return;
    }

    QUrl url("https://" + hostname_ + "/SpeedRating");
    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QString str = "time=" + strTimestamp + "&client_auth_hash=" + md5Hash + "&session_auth_hash="
                  + authHash + "&hostname=" + speedRatingHostname + "&rating="
                  + QString::number(rating) + "&ip=" + ip
                  + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString();

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, str.toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSpeedRating);
}

void ServerAPI::staticIps(const QString &authHash, const QString &deviceId, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit staticIpsAnswer(SERVER_RETURN_API_NOT_READY, apiinfo::StaticIps(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

#ifdef Q_OS_WIN
    QString strOs = "win";
#elif defined Q_OS_MAC
    QString strOs = "mac";
#elif defined Q_OS_LINUX
    QString strOs = "linux";
#endif

    qDebug() << "device_id for StaticIps request:" << deviceId;
    QUrl url("https://" + hostname_ + "/StaticIps?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&device_id=" + deviceId + "&os=" + strOs
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleStaticIps);
}

void ServerAPI::pingTest(quint64 cmdId, uint timeout, bool bWriteLog)
{
    const QString kCheckIpHostname = HardcodedSettings::instance().serverTunnelTestUrl();

    if (bWriteLog)
        qCDebug(LOG_SERVER_API) << "Do ping test to:" << kCheckIpHostname << " with timeout: " << timeout;

    QString strUrl = QString("https://") + kCheckIpHostname;
    QUrl url(strUrl);

    NetworkRequest request(url.toString(), timeout, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("cmdId", cmdId);
    reply->setProperty("isWriteLog", bWriteLog);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handlePingTest);
    pingTestReplies_[cmdId] = reply;
}

void ServerAPI::cancelPingTest(quint64 cmdId)
{
    auto it = pingTestReplies_.find(cmdId);
    if (it != pingTestReplies_.end()) {
        it.value()->deleteLater();
        pingTestReplies_.erase(it);
    }
}

void ServerAPI::notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit notificationsAnswer(SERVER_RETURN_API_NOT_READY, QVector<types::Notification>(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Notifications?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleNotifications);
}

void ServerAPI::wgConfigsInit(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit wgConfigsInitAnswer(SERVER_RETURN_API_NOT_READY, userRole, false, 0, QString(), QString());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/WgConfigs/init");
    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", authHash);
    // Must encode the public key in case it has '+' characters in its base64 encoding.  Otherwise the
    // server API will store the incorrect key and the wireguard handshake will fail due to a key mismatch.
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey));
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());
    if (deleteOldestKey)
        postData.addQueryItem("force_init", "1");

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleWgConfigsInit);
}

void ServerAPI::wgConfigsConnect(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled,
                                 const QString &clientPublicKey, const QString &serverName, const QString &deviceId)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit wgConfigsConnectAnswer(SERVER_RETURN_API_NOT_READY, userRole, false, 0, QString(), QString());
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/WgConfigs/connect");

    QUrlQuery postData;
    postData.addQueryItem("time", strTimestamp);
    postData.addQueryItem("client_auth_hash", md5Hash);
    postData.addQueryItem("session_auth_hash", authHash);
    // Must encode the public key in case it has '+' characters in its base64 encoding.  Otherwise the
    // server API will store the incorrect key and the wireguard handshake will fail due to a key mismatch.
    postData.addQueryItem("wg_pubkey", QUrl::toPercentEncoding(clientPublicKey));
    postData.addQueryItem("hostname", serverName);
    postData.addQueryItem("platform", Utils::getPlatformNameSafe());
    postData.addQueryItem("app_version", AppVersion::instance().semanticVersionString());

    if (!deviceId.isEmpty()) {
        qDebug() << "Setting device_id for WgConfigs connect request:" << deviceId;
        postData.addQueryItem("device_id", deviceId);
    }

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleWgConfigsConnect);
}

void ServerAPI::getRobertFilters(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit getRobertFiltersAnswer(SERVER_RETURN_API_NOT_READY, QVector<types::RobertFilter>(), userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Robert/filters?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    NetworkReply *reply = networkAccessManager_->get(request);
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleGetRobertFilters);
}

void ServerAPI::setRobertFilter(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter)
{
    if (isNeedCheckRequestsEnabled && !bIsRequestsEnabled_) {
        emit setRobertFilterAnswer(SERVER_RETURN_API_NOT_READY, userRole);
        return;
    }

    time_t timestamp;
    time(&timestamp);
    QString strTimestamp = QString::number(timestamp);
    QString strHash = HardcodedSettings::instance().serverSharedKey() + strTimestamp;
    QString md5Hash = QCryptographicHash::hash(strHash.toStdString().c_str(), QCryptographicHash::Md5).toHex();

    QUrl url("https://" + hostname_ + "/Robert/filter?time=" + strTimestamp
             + "&client_auth_hash=" + md5Hash + "&session_auth_hash=" + authHash
             + "&platform=" + Utils::getPlatformNameSafe() + "&app_version=" + AppVersion::instance().semanticVersionString());

    QString json = QString("{\"filter\":\"%1\", \"status\":%2}").arg(filter.id).arg(filter.status);

    NetworkRequest request(url.toString(), NETWORK_TIMEOUT, true, DnsServersConfiguration::instance().getCurrentDnsServers(), bIgnoreSslErrors_, currentProxySettings());
    request.setContentTypeHeader("Content-type: text/html; charset=utf-8");
    NetworkReply *reply = networkAccessManager_->put(request, json.toUtf8());
    reply->setProperty("userRole", userRole);
    connect(reply, &NetworkReply::finished, this, &ServerAPI::handleSetRobertFilter);
}

void ServerAPI::setIgnoreSslErrors(bool bIgnore)
{
    bIgnoreSslErrors_ = bIgnore;
}

void ServerAPI::handleAccessIps()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess())
    {
        SERVER_API_RET_CODE retCode;
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
        {
            retCode = SERVER_RETURN_SSL_ERROR;
        }
        else
        {
            retCode = SERVER_RETURN_NETWORK_ERROR;
        }

        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps failed(" << reply->errorString() << ")";
        emit accessIpsAnswer(retCode, QStringList(), userRole);
    }
    else
    {
        QByteArray arr = reply->readAll();
        //qCDebugMultiline(LOG_SERVER_API) << arr;

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

        const QJsonArray jsonArray = jsonData["hosts"].toArray();

        QStringList listHosts;
        for (const QJsonValue &value : jsonArray)
        {
            listHosts << value.toString();
        }
        qCDebug(LOG_SERVER_API) << "API request ApiAccessIps successfully executed";
        emit accessIpsAnswer(SERVER_RETURN_SUCCESS, listHosts, userRole);
    }
}

void ServerAPI::handleSessionReply()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    const uint userRole = reply->property("userRole").toUInt();
    const bool isLoginRequest = reply->property("isLoginRequest").toBool();

    if (!reply->isSuccess())
    {
        SERVER_API_RET_CODE retCode;

        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
        {
            retCode = SERVER_RETURN_SSL_ERROR;
        }
        else
        {
            retCode = SERVER_RETURN_NETWORK_ERROR;
        }

        if (isLoginRequest)
        {
            qCDebug(LOG_SERVER_API) << "API request Login failed(" << reply->errorString() << ")";
            emit loginAnswer(retCode, types::SessionStatus(), "", userRole, "");
        }
        else
        {
            qCDebug(LOG_SERVER_API) << "API request Session failed(" << reply->errorString() << ")";
            emit sessionAnswer(retCode, types::SessionStatus(), userRole);
        }
    }
    else
    {
        QByteArray arr = reply->readAll();

#ifdef TEST_API_FROM_FILES
        arr = SessionAndLocationsTest::instance().getSessionData();
#endif

        //qCDebugMultiline(LOG_SERVER_API) << arr;

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
            if (isLoginRequest)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json";
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), "", userRole, "");
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json";
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), userRole);
            }
            return;
        }
        QJsonObject jsonObject = doc.object();

        types::SessionStatus sessionStatus;

        if (jsonObject.contains("errorCode"))
        {
            int errorCode = jsonObject["errorCode"].toInt();

            // 701 - will be returned if the supplied session_auth_hash is invalid. Any authenticated endpoint can
            //       throw this error.  This can happen if the account gets disabled, or they rotate their session
            //       secret (pressed Delete Sessions button in the My Account section).  We should terminate the
            //       tunnel and go to the login screen.
            // 702 - will be returned ONLY in the login flow, and means the supplied credentials were not valid.
            //       Currently we disregard the API errorMessage and display the hardcoded ones (this is for
            //       multi-language support).
            // 703 - deprecated / never returned anymore, however we should still keep this for future purposes.
            //       If 703 is thrown on login (and only on login), display the exact errorMessage to the user,
            //       instead of what we do for 702 errors.
            // 706 - this is thrown only on login flow, and means the target account is disabled or banned.
            //       Do exactly the same thing as for 703 - show the errorMessage.

            if (errorCode == 701)
            {
                if (isLoginRequest)
                {
                    qCDebug(LOG_SERVER_API) << "API request Login return session auth hash invalid";
                    emit loginAnswer(SERVER_RETURN_SESSION_INVALID, types::SessionStatus(), "", userRole, "");
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << "API request Session return session auth hash invalid";
                    emit sessionAnswer(SERVER_RETURN_SESSION_INVALID, types::SessionStatus(), userRole);
                }
            }
            else if (errorCode == 702)
            {
                if (isLoginRequest)
                {
                    qCDebug(LOG_SERVER_API) << "API request Login return bad username";
                    emit loginAnswer(SERVER_RETURN_BAD_USERNAME, types::SessionStatus(), "", userRole, "");
                }
                else
                {
                    // According to the server API docs, we should not get here.
                    qCDebug(LOG_SERVER_API) << "WARNING: API request Session return bad username";
                    emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, types::SessionStatus(), userRole);
                }
            }
            else if (errorCode == 703 || errorCode == 706)
            {
                QString errorMessage = jsonObject["errorMessage"].toString();

                if (isLoginRequest)
                {
                    qCDebug(LOG_SERVER_API) << "API request Login return account disabled or banned";
                    emit loginAnswer(SERVER_RETURN_ACCOUNT_DISABLED, types::SessionStatus(), "", userRole, errorMessage);
                }
                else
                {
                    // According to the server API docs, we should not get here.
                    qCDebug(LOG_SERVER_API) << "WARNING: API request Session return account disabled or banned";
                    emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, types::SessionStatus(), userRole);
                }
            }
            else
            {
                if (isLoginRequest)
                {
                    if (errorCode == 1340)
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return missing 2FA code";
                        emit loginAnswer(SERVER_RETURN_MISSING_CODE2FA, types::SessionStatus(), "", userRole, "");
                    }
                    else if (errorCode == 1341)
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return invalid 2FA code";
                        emit loginAnswer(SERVER_RETURN_BAD_CODE2FA, types::SessionStatus(), "", userRole, "");
                    }
                    else
                    {
                        qCDebug(LOG_SERVER_API) << "API request Login return error";
                        emit loginAnswer(SERVER_RETURN_NETWORK_ERROR, types::SessionStatus(), "", userRole, "");
                    }
                }
                else
                {
                    qCDebug(LOG_SERVER_API) << "API request Session return error";
                    emit sessionAnswer(SERVER_RETURN_NETWORK_ERROR, types::SessionStatus(), userRole);
                }
            }
            return;
        }

        if (!jsonObject.contains("data"))
        {
            if (isLoginRequest)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json (data field not found)";
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), "", userRole, "");
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json (data field not found)";
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), userRole);
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
        bool success = sessionStatus.initFromJson(jsonData, outErrorMsg);
        if (!success)
        {
            if (isLoginRequest)
            {
                qCDebug(LOG_SERVER_API) << "API request Login incorrect json:" << outErrorMsg;
                emit loginAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), "", userRole, "");
            }
            else
            {
                qCDebug(LOG_SERVER_API) << "API request Session incorrect json:" << outErrorMsg;
                emit sessionAnswer(SERVER_RETURN_INCORRECT_JSON, types::SessionStatus(), userRole);
            }
        }

        if (isLoginRequest)
        {
            qCDebug(LOG_SERVER_API) << "API request Login successfully executed";
            emit loginAnswer(SERVER_RETURN_SUCCESS, sessionStatus, authHash, userRole, "");
        }
        else
        {
            // Commented debug entry out as this request may occur every minute and we don't
            // need to flood the log with this info.  Enabled it for staging builds to aid
            // QA in verifying session requests are being made when they're supposed to be.
            if (AppVersion::instance().isStaging())
            {
                qCDebug(LOG_SERVER_API) << "API request Session successfully executed";
            }
            emit sessionAnswer(SERVER_RETURN_SUCCESS, sessionStatus, userRole);
        }
    }
}

void ServerAPI::handleServerLocations()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();
    const bool isFromDisconnectedVPNState = reply->property("isFromDisconnectedVPNState").toBool();

    if (!reply->isSuccess())
    {
        qCDebug(LOG_SERVER_API) << "API request ServerLocations failed(" << reply->errorString() << ")";
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
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
        QByteArray arr = reply->readAll();

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
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (!jsonObject.contains("info"))
        {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (info field not found)";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }

        if (!jsonObject.contains("data"))
        {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json (data field not found)";
            emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<apiinfo::Location>(), QStringList(), userRole);
            return;
        }
        // parse revision number
        QJsonObject jsonInfo = jsonObject["info"].toObject();
        bool isChanged = jsonInfo["changed"].toInt() != 0;
        int newRevision = jsonInfo["revision"].toInt();
        QString revisionHash = jsonInfo["revision_hash"].toString();

        // manage the country override flag according to the documentation
        // https://gitlab.int.windscribe.com/ws/client/desktop/client-desktop-public/-/issues/354
        if (jsonInfo.contains("country_override"))
        {
            if (isFromDisconnectedVPNState && connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED)
            {
                QSettings settings;
                settings.setValue("countryOverride", jsonInfo["country_override"].toString());
                qCDebug(LOG_SERVER_API) << "API request ServerLocations saved countryOverride = " << jsonInfo["country_override"].toString();
            }
        }
        else
        {
            if (isFromDisconnectedVPNState && connectStateController_->currentState() == CONNECT_STATE::CONNECT_STATE_DISCONNECTED)
            {
                QSettings settings;
                settings.remove("countryOverride");
                qCDebug(LOG_SERVER_API) << "API request ServerLocations removed countryOverride flag";
            }
        }

        if (isChanged)
        {
            qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision changed =" << newRevision
                                    << ", revision_hash =" << revisionHash;

            // parse locations array
            const QJsonArray jsonData = jsonObject["data"].toArray();

            QVector<apiinfo::Location> serverLocations;
            QStringList forceDisconnectNodes;

            for (int i = 0; i < jsonData.size(); ++i)
            {
                if (jsonData.at(i).isObject())
                {
                    apiinfo::Location sl;
                    QJsonObject dataElement = jsonData.at(i).toObject();
                    if (sl.initFromJson(dataElement, forceDisconnectNodes)) {
                        serverLocations << sl;
                    }
                    else
                    {
                        QJsonDocument invalidData(dataElement);
                        qCDebug(LOG_SERVER_API) << "API request ServerLocations skipping invalid/incomplete 'data' element at index" << i
                                                << "(" << invalidData.toJson(QJsonDocument::Compact) << ")";
                    }
                }
                else {
                    qCDebug(LOG_SERVER_API) << "API request ServerLocations skipping non-object 'data' element at index" << i;
                }
            }

            if (serverLocations.empty())
            {
                qCDebugMultiline(LOG_SERVER_API) << arr;
                qCDebug(LOG_SERVER_API) << "API request ServerLocations incorrect json, no valid 'data' elements were found";
                emit serverLocationsAnswer(SERVER_RETURN_INCORRECT_JSON, serverLocations, QStringList(), userRole);
            }
            else {
                emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, serverLocations, forceDisconnectNodes, userRole);
            }
        }
        else
        {
            qCDebug(LOG_SERVER_API) << "API request ServerLocations successfully executed, revision not changed";
            emit serverLocationsAnswer(SERVER_RETURN_SUCCESS, QVector<apiinfo::Location>(), QStringList(), userRole);
        }
    }
}

void ServerAPI::handleServerCredentials()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();
    const PROTOCOL protocol = reply->property("protocol").toInt();

    if (!reply->isSuccess())
    {
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << "failed(" << reply->errorString() << ")";
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_) {
            emit serverCredentialsAnswer(SERVER_RETURN_SSL_ERROR, "", "", protocol, userRole);
        } else   {
            emit serverCredentialsAnswer(SERVER_RETURN_NETWORK_ERROR, "", "", protocol, userRole);
        }
    } else {
        QByteArray arr = reply->readAll();
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol.toShortString() << "incorrect json";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", protocol, userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol.toShortString() << "return error code:" << arr;
            emit serverCredentialsAnswer(SERVER_RETURN_SUCCESS, "", "", protocol, userRole);
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol.toShortString() << "incorrect json (data field not found)";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", protocol, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("username") || !jsonData.contains("password")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol.toShortString() << "incorrect json (username/password fields not found)";
            emit serverCredentialsAnswer(SERVER_RETURN_INCORRECT_JSON, "", "", protocol, userRole);
            return;
        }
        qCDebug(LOG_SERVER_API) << "API request ServerCredentials" << protocol.toShortString() << "successfully executed";
        emit serverCredentialsAnswer(SERVER_RETURN_SUCCESS,
                                     QByteArray::fromBase64(jsonData["username"].toString().toUtf8()),
                                     QByteArray::fromBase64(jsonData["password"].toString().toUtf8()), protocol, userRole);
    }
}

void ServerAPI::handleDeleteSession()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    if (!reply->isSuccess())
        qCDebug(LOG_SERVER_API) << "API request DeleteSession failed(" << reply->errorString() << ")";
    else
        qCDebug(LOG_SERVER_API) << "API request DeleteSession successfully executed";
}

void ServerAPI::handleServerConfigs()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "API request ServerConfigs failed(" << reply->errorString() << ")";
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
            emit serverConfigsAnswer(SERVER_RETURN_SSL_ERROR, QByteArray(), userRole);
        else
            emit serverConfigsAnswer(SERVER_RETURN_NETWORK_ERROR, QByteArray(), userRole);
    } else {
        qCDebug(LOG_SERVER_API) << "API request ServerConfigs successfully executed";
        QByteArray ovpnConfig = QByteArray::fromBase64(reply->readAll());
        emit serverConfigsAnswer(SERVER_RETURN_SUCCESS, ovpnConfig, userRole);
    }
}

void ServerAPI::handlePortMap()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "API request PortMap failed(" << reply->errorString() << ")";
        if (reply->error() ==  NetworkReply::NetworkError::SslError && !bIgnoreSslErrors_)
            emit portMapAnswer(SERVER_RETURN_SSL_ERROR, types::PortMap(), userRole);
        else
            emit portMapAnswer(SERVER_RETURN_NETWORK_ERROR, types::PortMap(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, types::PortMap(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (data field not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, types::PortMap(), userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("portmap")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap field not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, types::PortMap(), userRole);
            return;
        }

        QJsonArray jsonArr = jsonData["portmap"].toArray();

        types::PortMap portMap;
        if (!portMap.initFromJson(jsonArr)) {
            qCDebug(LOG_SERVER_API) << "API request PortMap incorrect json (portmap required fields not found)";
            emit portMapAnswer(SERVER_RETURN_INCORRECT_JSON, types::PortMap(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "API request PortMap successfully executed";
        emit portMapAnswer(SERVER_RETURN_SUCCESS, portMap, userRole);
    }
}

void ServerAPI::handleMyIP()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    WS_ASSERT(reply == myIpReply_);
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    myIpReply_ = nullptr;
    const uint userRole = reply->property("userRole").toUInt();
    const bool isDisconnected = reply->property("isDisconnected").toBool();

    if (!reply->isSuccess()) {
        emit myIPAnswer("N/A", false, isDisconnected, userRole);
        return;
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(data field not found)";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("user_ip")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "API request MyIP incorrect json(user_ip field not found)";
            emit myIPAnswer("N/A", false, isDisconnected, userRole);
        }
        emit myIPAnswer(jsonData["user_ip"].toString(), true, isDisconnected, userRole);
    }
}

void ServerAPI::handleCheckUpdate()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Check update failed(" << reply->errorString() << ")";
        emit checkUpdateAnswer(types::CheckUpdate(), true, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            // Putting this debug line here to help us quickly troubleshoot errors returned from the
            // server API, which hopefully should be few and far between.
            qCDebugMultiline(LOG_SERVER_API) << arr;
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("update_needed_flag")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        int updateNeeded = jsonData["update_needed_flag"].toInt();
        if (updateNeeded != 1) {
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        if (!jsonData.contains("latest_version")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }
        if (!jsonData.contains("update_url")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for CheckUpdate";
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        QString err;
        bool outSuccess;
        types::CheckUpdate cu = types::CheckUpdate::createFromApiJson(jsonData, outSuccess, err);
        if (!outSuccess) {
            qCDebug(LOG_SERVER_API) << err;
            emit checkUpdateAnswer(types::CheckUpdate(), false, userRole);
            return;
        }

        emit checkUpdateAnswer(cu, false, userRole);
    }
}

void ServerAPI::handleRecordInstall()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess())
        qCDebug(LOG_SERVER_API) << "RecordInstall request failed(" << reply->errorString() << ")";
    else {
        qCDebug(LOG_SERVER_API) << "RecordInstall request successfully executed";
        //QByteArray arr = reply->readAll();
        //qCDebugMultiline(LOG_SERVER_API) << arr;
    }
}

void ServerAPI::handleConfirmEmail()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Users request failed(" << reply->errorString() << ")";
        emit confirmEmailAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    } else {
        QByteArray arr = reply->readAll();
        //qCDebugMultiline(LOG_SERVER_API) << arr;
        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("email_sent")) {
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Users";
            emit confirmEmailAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        int emailSent = jsonData["email_sent"].toInt();
        emit confirmEmailAnswer(emailSent == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleDebugLog()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "DebugLog request failed(" << reply->errorString() << ")";
        emit debugLogAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("success")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Report";
            emit debugLogAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        int is_success = jsonData["success"].toInt();
        emit debugLogAnswer(is_success == 1 ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
    }
}

void ServerAPI::handleSpeedRating()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);

    if (!reply->isSuccess())
        qCDebug(LOG_SERVER_API) << "SpeedRating request failed(" << reply->errorString() << ")";
    else {
        qCDebug(LOG_SERVER_API) << "SpeedRating request successfully executed";
        QByteArray arr = reply->readAll();
        qCDebugMultiline(LOG_SERVER_API) << arr;
    }
}

void ServerAPI::handlePingTest()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const quint64 cmdId = reply->property("cmdId").toULongLong();
    const bool isWriteLog = reply->property("isWriteLog").toBool();

    pingTestReplies_.remove(cmdId);

    if (!reply->isSuccess()) {
        if (isWriteLog)
            qCDebug(LOG_SERVER_API) << "PingTest request failed(" << reply->errorString() << ")";
        emit pingTestAnswer(SERVER_RETURN_NETWORK_ERROR, "");
    } else {
        QByteArray arr = reply->readAll();
        emit pingTestAnswer(SERVER_RETURN_SUCCESS, arr);
    }
}

void ServerAPI::handleNotifications()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Notifications request failed(" << reply->errorString() << ")";
        emit notificationsAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<types::Notification>(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications";
            emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        const QJsonArray jsonNotifications = jsonData["notifications"].toArray();

        QVector<types::Notification> notifications;

        for (const QJsonValue &value : jsonNotifications) {
            QJsonObject obj = value.toObject();
            types::Notification n;
            if (!n.initFromJson(obj)) {
                qCDebug(LOG_SERVER_API) << "Failed parse JSON for Notifications (not all required fields)";
                emit notificationsAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::Notification>(), userRole);
                return;
            }
            notifications.push_back(n);
        }
        qCDebug(LOG_SERVER_API) << "Notifications request successfully executed";
        emit notificationsAnswer(SERVER_RETURN_SUCCESS, notifications, userRole);
    }
}

void ServerAPI::handleWgConfigsInit()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "WgConfigs init curl request failed(" << reply->errorString() << ")";
        emit wgConfigsInitAnswer(SERVER_RETURN_NETWORK_ERROR, userRole, false, 0, QString(), QString());
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs init response";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            int errorCode = jsonObject["errorCode"].toInt();
            qCDebug(LOG_SERVER_API) << "WgConfigs init failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode << ")";
            emit wgConfigsInitAnswer(SERVER_RETURN_SUCCESS, userRole, true, errorCode, QString(), QString());
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init JSON is missing the 'data' element";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonData = jsonObject["data"].toObject();
        if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init JSON contains a missing or invalid 'success' field";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonConfig = jsonData["config"].toObject();
        if (jsonConfig.contains("PresharedKey") && jsonConfig.contains("AllowedIPs")) {
            QString presharedKey = jsonConfig["PresharedKey"].toString();
            QString allowedIps   = jsonConfig["AllowedIPs"].toString();
            qCDebug(LOG_SERVER_API) << "WgConfigs/init json:" << doc.toJson(QJsonDocument::Compact);
            qCDebug(LOG_SERVER_API) << "WgConfigs init request successfully executed";
            emit wgConfigsInitAnswer(SERVER_RETURN_SUCCESS, userRole, false, 0, presharedKey, allowedIps);
        } else {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs init 'config' entry is missing required elements";
            emit wgConfigsInitAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
        }
    }
}

void ServerAPI::handleWgConfigsConnect()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "WgConfigs connect curl request failed(" << reply->errorString() << ")";
        emit wgConfigsConnectAnswer(SERVER_RETURN_NETWORK_ERROR, userRole, false, 0, QString(), QString());
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed to parse JSON for WgConfigs connect response";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonObject = doc.object();

        if (jsonObject.contains("errorCode")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            int errorCode = jsonObject["errorCode"].toInt();
            qCDebug(LOG_SERVER_API) << "WgConfigs connect failed:" << jsonObject["errorMessage"].toString() << "(" << errorCode << ")";
            emit wgConfigsConnectAnswer(SERVER_RETURN_SUCCESS, userRole, true, errorCode, QString(), QString());
            return;
        }

        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON is missing the 'data' element";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonData = jsonObject["data"].toObject();
        if (!jsonData.contains("success") || jsonData["success"].toInt(0) == 0) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect JSON contains a missing or invalid 'success' field";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
            return;
        }

        QJsonObject jsonConfig = jsonData["config"].toObject();
        if (jsonConfig.contains("Address") && jsonConfig.contains("DNS")) {
            QString ipAddress  = jsonConfig["Address"].toString();
            QString dnsAddress = jsonConfig["DNS"].toString();
            qCDebug(LOG_SERVER_API) << "WgConfigs connect request successfully executed";
            emit wgConfigsConnectAnswer(SERVER_RETURN_SUCCESS, userRole, false, 0, ipAddress, dnsAddress);

        } else {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "WgConfigs connect 'config' entry is missing required elements";
            emit wgConfigsConnectAnswer(SERVER_RETURN_INCORRECT_JSON, userRole, false, 0, QString(), QString());
        }
    }
}

void ServerAPI::handleWebSession()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "API request WebSession failed(" << reply->errorString() << ")";
        emit webSessionAnswer(SERVER_RETURN_NETWORK_ERROR, QString(), userRole);
    } else {
        QByteArray arr = reply->readAll();
        qCDebugMultiline(LOG_SERVER_API) << arr;

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json";
            emit webSessionAnswer(SERVER_RETURN_INCORRECT_JSON, QString(), userRole);
            return;
        }
        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json (data field not found)";
            emit webSessionAnswer(SERVER_RETURN_INCORRECT_JSON, QString(), userRole);
            return;
        }
        QJsonObject jsonData =  jsonObject["data"].toObject();
        if (!jsonData.contains("temp_session")) {
            qCDebug(LOG_SERVER_API) << "API request WebSession incorrect json (temp_session field not found)";
            emit webSessionAnswer(SERVER_RETURN_INCORRECT_JSON, QString(), userRole);
            return;
        }

        const QString temp_session_token = jsonData["temp_session"].toString();

        qCDebug(LOG_SERVER_API) << "API request WebSession successfully executed";
        emit webSessionAnswer(SERVER_RETURN_SUCCESS, temp_session_token, userRole);
    }
}

void ServerAPI::handleGetRobertFilters()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Get ROBERT filters request failed(" << reply->errorString() << ")";
        emit getRobertFiltersAnswer(SERVER_RETURN_NETWORK_ERROR, QVector<types::RobertFilter>(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
            emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters";
            emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        const QJsonArray jsonFilters = jsonData["filters"].toArray();

        QVector<types::RobertFilter> filters;

        for (const QJsonValue &value : jsonFilters) {
            QJsonObject obj = value.toObject();

            types::RobertFilter f;
            if (!f.initFromJson(obj)) {
                qCDebug(LOG_SERVER_API) << "Failed parse JSON for get ROBERT filters (not all required fields)";
                emit getRobertFiltersAnswer(SERVER_RETURN_INCORRECT_JSON, QVector<types::RobertFilter>(), userRole);
                return;
            }

            filters.push_back(f);
        }
        qCDebug(LOG_SERVER_API) << "Get ROBERT request successfully executed";
        emit getRobertFiltersAnswer(SERVER_RETURN_SUCCESS, filters, userRole);
    }
}

void ServerAPI::handleSetRobertFilter()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "Set ROBERT filter request failed(" << reply->errorString() << ")";
        emit setRobertFilterAnswer(SERVER_RETURN_NETWORK_ERROR, userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
            emit setRobertFilterAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for set ROBERT filters";
            emit setRobertFilterAnswer(SERVER_RETURN_INCORRECT_JSON, userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();
        qCDebug(LOG_SERVER_API) << "Set ROBERT request successfully executed ( result =" << jsonData["success"] << ")";
        emit setRobertFilterAnswer((jsonData["success"] == 1) ? SERVER_RETURN_SUCCESS : SERVER_RETURN_INCORRECT_JSON, userRole);
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
void ServerAPI::handleStaticIps()
{
    NetworkReply *reply = static_cast<NetworkReply *>(sender());
    QSharedPointer<NetworkReply> obj = QSharedPointer<NetworkReply>(reply, &QObject::deleteLater);
    const uint userRole = reply->property("userRole").toUInt();

    if (!reply->isSuccess()) {
        qCDebug(LOG_SERVER_API) << "StaticIps request failed(" << reply->errorString() << ")";
        emit staticIpsAnswer(SERVER_RETURN_NETWORK_ERROR, apiinfo::StaticIps(), userRole);
    } else {
        QByteArray arr = reply->readAll();

        QJsonParseError errCode;
        QJsonDocument doc = QJsonDocument::fromJson(arr, &errCode);
        if (errCode.error != QJsonParseError::NoError || !doc.isObject()) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonObject = doc.object();
        if (!jsonObject.contains("data")) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        QJsonObject jsonData =  jsonObject["data"].toObject();

        apiinfo::StaticIps staticIps;
        if (!staticIps.initFromJson(jsonData)) {
            qCDebugMultiline(LOG_SERVER_API) << arr;
            qCDebug(LOG_SERVER_API) << "Failed parse JSON for StaticIps";
            emit staticIpsAnswer(SERVER_RETURN_INCORRECT_JSON, apiinfo::StaticIps(), userRole);
            return;
        }

        qCDebug(LOG_SERVER_API) << "StaticIps request successfully executed";
        emit staticIpsAnswer(SERVER_RETURN_SUCCESS, staticIps, userRole);
    }
}
