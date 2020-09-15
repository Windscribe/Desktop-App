#ifndef ICONNECTIONMANAGER_H
#define ICONNECTIONMANAGER_H

#include <QObject>
#include "engine/customovpnconfigs/customovpnauthcredentialsstorage.h"
#include "automanualconnectioncontroller.h"
#include "engine/types/types.h"
#include "engine/types/protocoltype.h"

class IHelper;
class INetworkStateManager;
class ProxySettings;
class ServerAPI;
class ServerNode;
class MutableLocationInfo;
class WireGuardConfig;
struct ConnectionSettings;
struct PortMap;

// manage openvpn connection, reconnects, sleep mode, network change, automatic/manual connection mode
class IConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit IConnectionManager(QObject *parent, IHelper *helper, INetworkStateManager *networkStateManager, ServerAPI *serverAPI, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage) : QObject(parent),
        helper_(helper), networkStateManager_(networkStateManager), customOvpnAuthCredentialsStorage_(customOvpnAuthCredentialsStorage) { Q_UNUSED(serverAPI); }
    virtual ~IConnectionManager() {}

    virtual void clickConnect(const QByteArray &ovpnConfig, const ServerCredentials &serverCredentials,
                              QSharedPointer<MutableLocationInfo> mli,
                              const ConnectionSettings &connectionSettings, const PortMap &portMap, const ProxySettings &proxySettings,
                              bool bEmitAuthError) = 0;

    virtual void clickDisconnect() = 0;
    virtual void blockingDisconnect() = 0;
    virtual bool isDisconnected() = 0;
    virtual QString getLastConnectedIp() = 0;

    virtual void removeIkev2ConnectionFromOS() = 0;

    virtual void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect) = 0;
    virtual void continueWithPassword(const QString &password, bool bNeedReconnect) = 0;

    virtual bool isCustomOvpnConfigCurrentConnection() const = 0;
    virtual QString getCustomOvpnConfigFilePath() = 0;

    virtual bool isStaticIpsLocation() const = 0;
    virtual StaticIpPortsVector getStatisIps() = 0;

    virtual QString getConnectedTapTunAdapter() = 0;

    virtual void setWireGuardConfig(QSharedPointer<WireGuardConfig> config) = 0;

    //windows specific functions
    virtual void setMss(int mss) = 0;

signals:
    void connected();
    void connectingToHostname(const QString &hostname);
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(CONNECTION_ERROR errorCode);
    void reconnecting();
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void testTunnelResult(bool success, const QString &ipAddress);
    void showFailedAutomaticConnectionMessage();
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);
    void getWireGuardConfig();

    void requestUsername(const QString &pathCustomOvpnConfig);
    void requestPassword(const QString &pathCustomOvpnConfig);


protected:
    IHelper *helper_;
    INetworkStateManager *networkStateManager_;
    AutoManualConnectionController autoManualConnectionController_;
    CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage_;
};

#endif // ICONNECTIONMANAGER_H
