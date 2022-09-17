#pragma once

#include <QObject>

#include "types/notification.h"
#include "types/portmap.h"
#include "engine/apiinfo/staticips.h"
#include "types/checkupdate.h"
#include "types/proxysettings.h"
#include "types/sessionstatus.h"
#include "engine/apiinfo/location.h"
#include "types/robertfilter.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"

class INetworkStateManager;
class IConnectStateController;

// access to API endpoint with custom DNS-resolution
class ServerAPI : public QObject
{
    Q_OBJECT
public:
    class BaseRequest;

    explicit ServerAPI(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);
    virtual ~ServerAPI();

    uint getAvailableUserRole();

    void setProxySettings(const types::ProxySettings &proxySettings);
    void disableProxy();
    void enableProxy();

    // if true, then all requests works
    // if false, then only request for SERVER_API_ROLE_LOGIN_CONTROLLER and SERVER_API_ROLE_ACCESS_IPS_CONTROLLER roles works
    void setRequestsEnabled(bool bEnable);
    bool isRequestsEnabled() const;

    // set single hostname for make API requests
    void setHostname(const QString &hostname);
    QString getHostname() const;

    void accessIps(const QString &hostIp, uint userRole);
    void login(const QString &username, const QString &password, const QString &code2fa, uint userRole);
    void session(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void serverLocations(const QString &authHash, const QString &language, uint userRole, bool isNeedCheckRequestsEnabled,
                         const QString &revision, bool isPro, PROTOCOL protocol, const QStringList &alcList);
    void serverCredentials(const QString &authHash, uint userRole, PROTOCOL protocol, bool isNeedCheckRequestsEnabled);
    void deleteSession(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void serverConfigs(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void portMap(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void recordInstall(uint userRole, bool isNeedCheckRequestsEnabled);
    void confirmEmail(uint userRole, const QString &authHash, bool isNeedCheckRequestsEnabled);
    void webSession(const QString authHash, uint userRole, bool isNeedCheckRequestsEnabled);

    void myIP(bool isDisconnected, uint userRole, bool isNeedCheckRequestsEnabled);

    void checkUpdate(UPDATE_CHANNEL updateChannel, uint userRole, bool isNeedCheckRequestsEnabled);
    void debugLog(const QString &username, const QString &strLog, uint userRole, bool isNeedCheckRequestsEnabled);
    void speedRating(const QString &authHash, const QString &speedRatingHostname, const QString &ip, int rating,
                     uint userRole, bool isNeedCheckRequestsEnabled);

    void staticIps(const QString &authHash, const QString &deviceId, uint userRole, bool isNeedCheckRequestsEnabled);

    void pingTest(quint64 cmdId, uint timeout, bool bWriteLog);
    void cancelPingTest(quint64 cmdId);

    void notifications(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);

    void getRobertFilters(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled);
    void setRobertFilter(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const types::RobertFilter &filter);

    void wgConfigsInit(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, bool deleteOldestKey);
    void wgConfigsConnect(const QString &authHash, uint userRole, bool isNeedCheckRequestsEnabled, const QString &clientPublicKey, const QString &serverName, const QString &deviceId);

    void setIgnoreSslErrors(bool bIgnore);

signals:
    void accessIpsAnswer(SERVER_API_RET_CODE retCode, const QStringList &hosts, uint userRole);
    void loginAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, const QString &authHash,
                     uint userRole, const QString &errorMessage);
    void sessionAnswer(SERVER_API_RET_CODE retCode, const types::SessionStatus &sessionStatus, uint userRole);
    void serverLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Location> &serverLocations,
                               QStringList forceDisconnectNodes, uint userRole);
    void serverCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername,
                                 const QString &radiusPassword, PROTOCOL protocol, uint userRole);
    void serverConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole);
    void portMapAnswer(SERVER_API_RET_CODE retCode, const types::PortMap &portMap, uint userRole);
    void myIPAnswer(const QString &ip, bool success, bool isDisconnected, uint userRole);
    void checkUpdateAnswer(const types::CheckUpdate &checkUpdate, bool bNetworkErrorOccured, uint userRole);
    void debugLogAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void confirmEmailAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void staticIpsAnswer(SERVER_API_RET_CODE retCode, const apiinfo::StaticIps &staticIps, uint userRole);
    void pingTestAnswer(SERVER_API_RET_CODE retCode, const QString &data);
    void notificationsAnswer(SERVER_API_RET_CODE retCode, QVector<types::Notification> notifications, uint userRole);

    void wgConfigsInitAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &presharedKey, const QString &allowedIps);
    void wgConfigsConnectAnswer(SERVER_API_RET_CODE retCode, uint userRole, bool isErrorCode, int errorCode, const QString &ipAddress, const QString &dnsAddress);

    void webSessionAnswer(SERVER_API_RET_CODE retCode, const QString &token, uint userRole);
    void sendUserWarning(USER_WARNING_TYPE warning);
    void getRobertFiltersAnswer(SERVER_API_RET_CODE retCode, const QVector<types::RobertFilter> &robertFilters, uint userRole);
    void setRobertFilterAnswer(SERVER_API_RET_CODE retCode, uint userRole);

private:
    NetworkAccessManager *networkAccessManager_;
    types::ProxySettings proxySettings_;
    bool isProxyEnabled_;

    enum {
        GET_MY_IP_TIMEOUT = 5000,
        NETWORK_TIMEOUT = 10000,
    };

    void handleAccessIps();
    void handleSessionReply();
    void handleServerLocations();
    void handleServerCredentials();
    void handleDeleteSession();
    void handleServerConfigs();
    void handlePortMap();
    void handleMyIP();
    void handleCheckUpdate();
    void handleRecordInstall();
    void handleConfirmEmail();
    void handleDebugLog();
    void handleSpeedRating();
    void handlePingTest();
    void handleNotifications();
    void handleStaticIps();
    void handleWgConfigsInit();
    void handleWgConfigsConnect();
    void handleWebSession();
    void handleGetRobertFilters();
    void handleSetRobertFilter();

    IConnectStateController *connectStateController_;

    QString hostname_;
    QStringList hostIps_;

    enum HOST_MODE { HOST_MODE_HOSTNAME, HOST_MODE_IPS };
    HOST_MODE hostMode_;

    bool bIsRequestsEnabled_;
    uint curUserRole_;
    bool bIgnoreSslErrors_;

    NetworkReply *myIpReply_;
    QHash<quint64, NetworkReply *> pingTestReplies_;

    types::ProxySettings currentProxySettings() const;
};

