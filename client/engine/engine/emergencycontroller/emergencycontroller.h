#pragma once

#include <QHostInfo>
#include <QObject>
#include <wsnet/WSNet.h>
#include "engine/helper/ihelper.h"
#include "types/enums.h"
#include "types/packetsize.h"
#include "engine/connectionmanager/iconnection.h"
#include "engine/connectionmanager/makeovpnfile.h"

#ifdef Q_OS_MAC
    #include "engine/connectionmanager/restorednsmanager_mac.h"
#endif

// for manage connection to Emergency Server
class EmergencyController : public QObject
{
    Q_OBJECT
public:
    explicit EmergencyController(QObject *parent, IHelper *helper);
    virtual ~EmergencyController();

    void clickConnect(const types::ProxySettings &proxySettings);
    void clickDisconnect();
    bool isDisconnected();
    void blockingDisconnect();

    const AdapterGatewayInfo &getVpnAdapterInfo() const;

    void setPacketSize(types::PacketSize ps);

signals:
    void connected();
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(CONNECT_ERROR errorCode);

private slots:
    void onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo);
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(CONNECT_ERROR err);

private:
    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_ERROR_DURING_CONNECTION};

    IHelper *helper_;
    IConnection *connector_;
    MakeOVPNFile *makeOVPNFile_;
    types::ProxySettings proxySettings_;

    std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> endpoints_;

    QString lastIp_;
    int state_;
    types::PacketSize packetSize_;

    AdapterGatewayInfo defaultAdapterInfo_;
    AdapterGatewayInfo vpnAdapterInfo_;

    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;

    void doConnect();
    void doMacRestoreProcedures();
};
