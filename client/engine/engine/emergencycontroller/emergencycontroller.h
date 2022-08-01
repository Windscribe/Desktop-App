#ifndef EMERGENCYCONTROLLER_H
#define EMERGENCYCONTROLLER_H

#include <QHostInfo>
#include <QObject>
#include "engine/helper/ihelper.h"
#include "types/enums.h"
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

    void setPacketSize(ProtoTypes::PacketSize ps);

signals:
    void connected();
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(ProtoTypes::ConnectError errorCode);

private slots:
    void onDnsRequestFinished();

    void onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo);
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(ProtoTypes::ConnectError err);

private:
    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_ERROR_DURING_CONNECTION};

    IHelper *helper_;
    QByteArray ovpnConfig_;
    IConnection *connector_;
    MakeOVPNFile *makeOVPNFile_;
    types::ProxySettings proxySettings_;

    struct CONNECT_ATTEMPT_INFO
    {
        QString ip;
        uint port;
        QString protocol;   //udp or tcp
    };
    QVector<CONNECT_ATTEMPT_INFO> attempts_;

    QString lastIp_;
    uint serverApiUserRole_;
    int state_;
    ProtoTypes::PacketSize packetSize_;

    AdapterGatewayInfo defaultAdapterInfo_;
    AdapterGatewayInfo vpnAdapterInfo_;

    void doConnect();
    void doMacRestoreProcedures();
    void addRandomHardcodedIpsToAttempts();
};

#endif // EMERGENCYCONTROLLER_H
