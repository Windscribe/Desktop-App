#ifndef SERVERAPI_H
#define SERVERAPI_H

#include <QObject>
#include <QLinkedList>
#include <QTimer>
#include "engine/types/wireguardconfig.h"
#include "engine/apiinfo/apiinfo.h"
#include "engine/apiinfo/notification.h"
#include "engine/apiinfo/portmap.h"
#include "engine/apiinfo/staticips.h"
#include "engine/proxy/proxysettings.h"
#include "dnscache.h"
#include "curlnetworkmanager.h"

class INetworkStateManager;

// access to API endpoint with custom DNS-resolution
class ServerAPI : public QObject
{
    Q_OBJECT
public:
    class BaseRequest;

    explicit ServerAPI(QObject *parent, INetworkStateManager *networkStateManager);
    virtual ~ServerAPI();

    uint getAvailableUserRole();

    void setProxySettings(const ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    // if true, then all requests works
    // if false, then only request for SERVER_API_ROLE_LOGIN_CONTROLLER and SERVER_API_ROLE_ACCESS_IPS_CONTROLLER roles works
    void setRequestsEnabled(bool bEnable);
    bool isRequestsEnabled() const;

    // set single hostname for make API requests
    void setHostname(const QString &hostname);
    QString getHostname() const { return hostname_; }

    void accessIps(const QString &hostIp, uint userRole, bool isNeedCheckRequestsEnabled);
    void login(const QString &username, const QString &password, const QString &code2fa, uint userRole, bool isNeedCheckRequestsEnabled);
    void session(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                         const QString &revision, bool isPro, ProtocolType protocol, const QStringList &alcList);
    void serverCredentials(const QString &authHash, uint userRole, ProtocolType protocol, bool isNeedCheckRequestsEnabled);
    void deleteSession(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void serverConfigs(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void portMap(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void recordInstall(uint userRole, bool isNeedCheckRequestsEnabled);
    void confirmEmail(uint userRole, const QString &authHash, bool isNeedCheckRequestsEnabled);

    void myIP(bool isDisconnected, uint userRole, bool isNeedCheckRequestsEnabled);

    void checkUpdate(const ProtoTypes::UpdateChannel updateChannel, uint userRole, bool isNeedCheckRequestsEnabled);
    void debugLog(const QString &username, const QString &strLog, uint userRole, bool isNeedCheckRequestsEnabled);
    void speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating, uint userRole, bool isNeedCheckRequestsEnabled);

    void staticIps(const QString &authHash, const QString &deviceId, uint userRole, bool isNeedCheckRequestsEnabled);

    void pingTest(quint64 cmdId, int testNum);
    void cancelPingTest(quint64 cmdId);

    void notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void getWireGuardConfig(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);

    bool isOnline();

    void setIgnoreSslErrors(bool bIgnore);

signals:
    void accessIpsAnswer(SERVER_API_RET_CODE retCode, const QStringList &hosts, uint userRole);
    void loginAnswer(SERVER_API_RET_CODE retCode, const apiinfo::SessionStatus &sessionStatus, const QString &authHash, uint userRole);
    void sessionAnswer(SERVER_API_RET_CODE retCode, const apiinfo::SessionStatus &sessionStatus, uint userRole);
    void serverLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Location> &serverLocations,
                               QStringList forceDisconnectNodes, uint userRole);
    void serverCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername,
                                 const QString &radiusPassword, ProtocolType protocol, uint userRole);
    void serverConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole);
    void portMapAnswer(SERVER_API_RET_CODE retCode, const apiinfo::PortMap &portMap, uint userRole);
    void myIPAnswer(const QString &ip, bool success, bool isDisconnected, uint userRole);
    void checkUpdateAnswer(bool bAvailable, const QString &version, const ProtoTypes::UpdateChannel updateChannel, int latestBuild, const QString &url, bool supported, bool bNetworkErrorOccured, uint userRole);
    void debugLogAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void confirmEmailAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void staticIpsAnswer(SERVER_API_RET_CODE retCode, const apiinfo::StaticIps &staticIps, uint userRole);
    void pingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void notificationsAnswer(SERVER_API_RET_CODE retCode, QVector<apiinfo::Notification> notifications, uint userRole);
    void getWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<WireGuardConfig> config, uint userRole);

    // need for add to firewall rules
    void hostIpsChanged(const QStringList &hostIps);

private slots:
    void onDnsResolved(bool success, void *userData, const QStringList &ips);
    void onCurlNetworkRequestFinished(CurlRequest *curlRequest);
    void onNetworkAccessibleChanged(bool isOnline);
    void onRequestTimer();

private:
    using HandleDnsResolveFunc = void (ServerAPI::*)(BaseRequest*,bool, const QStringList&);
    using HandleCurlReplyFunc = void (ServerAPI::*)(BaseRequest*, bool);

    enum {
        PING_TEST_TIMEOUT_1 = 2000,
        PING_TEST_TIMEOUT_2 = 4000,
        PING_TEST_TIMEOUT_3 = 8000,
        GET_MY_IP_TIMEOUT = 5000,
        NETWORK_TIMEOUT = 10000,
    };
    enum {
        REPLY_ACCESS_IPS,
        REPLY_LOGIN,
        REPLY_SESSION,
        REPLY_SERVER_LOCATIONS,
        REPLY_SERVER_CREDENTIALS,
        REPLY_DELETE_SESSION,
        REPLY_SERVER_CONFIGS,
        REPLY_PORT_MAP,
        REPLY_MY_IP,
        REPLY_CHECK_UPDATE,
        REPLY_RECORD_INSTALL,
        REPLY_DEBUG_LOG,
        REPLY_SPEED_RATING,
        REPLY_PING_TEST,
        REPLY_NOTIFICATIONS,
        REPLY_STATIC_IPS,
        REPLY_CONFIRM_EMAIL,
        REPLY_WIREGUARD_CONFIG,
        NUM_REPLY_TYPES
    };

    template<typename RequestType, typename... RequestArgs>
    RequestType *createRequest(RequestArgs &&... args) {
        auto *request = new RequestType(std::forward<RequestArgs>( args )...);
        if (request)
            activeRequests_.append(request);
        return request;
    }
    void submitDnsRequest(BaseRequest *request);
    void submitCurlRequest(BaseRequest *request, CurlRequest::MethodType type,
                           const QString &contentTypeHeader, const QString &hostname,
                           const QStringList &ips);

    void handleRequestTimeout(BaseRequest *rd);

    void handleLoginDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleSessionDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleServerLocationsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleServerCredentialsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleDeleteSessionDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleServerConfigsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handlePortMapDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleRecordInstallDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleConfirmEmailDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleMyIPDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleCheckUpdateDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleDebugLogDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleSpeedRatingDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleNotificationsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleStaticIpsDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handlePingTestDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);
    void handleWireGuardConfigDnsResolve(BaseRequest *rd, bool success, const QStringList &ips);

    void handleAccessIpsCurl(BaseRequest *rd, bool success);
    void handleSessionReplyCurl(BaseRequest *rd, bool success);
    void handleServerLocationsCurl(BaseRequest *rd, bool success);
    void handleServerCredentialsCurl(BaseRequest *rd, bool success);
    void handleDeleteSessionCurl(BaseRequest *rd, bool success);
    void handleServerConfigsCurl(BaseRequest *rd, bool success);
    void handlePortMapCurl(BaseRequest *rd, bool success);
    void handleMyIPCurl(BaseRequest *rd, bool success);
    void handleCheckUpdateCurl(BaseRequest *rd, bool success);
    void handleRecordInstallCurl(BaseRequest *rd, bool success);
    void handleConfirmEmailCurl(BaseRequest *rd, bool success);
    void handleDebugLogCurl(BaseRequest *rd, bool success);
    void handleSpeedRatingCurl(BaseRequest *rd, bool success);
    void handlePingTestCurl(BaseRequest *rd, bool success);
    void handleNotificationsCurl(BaseRequest *rd, bool success);
    void handleStaticIpsCurl(BaseRequest *rd, bool success);
    void handleWireGuardConfigCurl(BaseRequest *rd, bool success);

    INetworkStateManager *networkStateManager_;
    CurlNetworkManager curlNetworkManager_;

    bool bIsOnline_;

    QString lastLocationsLanguage_;

    DnsCache *dnsCache_;

    QString hostname_;
    QStringList hostIps_;

    enum HOST_MODE { HOST_MODE_HOSTNAME, HOST_MODE_IPS };
    HOST_MODE hostMode_;

    bool bIsRequestsEnabled_;
    uint curUserRole_;
    bool bIgnoreSslErrors_;

    QLinkedList<BaseRequest*> activeRequests_;
    QMap<const CurlRequest*, BaseRequest*> curlToRequestMap_;
    HandleDnsResolveFunc handleDnsResolveFuncTable_[NUM_REPLY_TYPES];
    HandleCurlReplyFunc handleCurlReplyFuncTable_[NUM_REPLY_TYPES];
    QTimer requestTimer_;
};

#endif // SERVERAPI_H
