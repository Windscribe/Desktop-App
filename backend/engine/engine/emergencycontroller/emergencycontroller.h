#ifndef EMERGENCYCONTROLLER_H
#define EMERGENCYCONTROLLER_H

#include <QHostInfo>
#include <QObject>
#include "engine/helper/ihelper.h"
#include "engine/types/types.h"
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

    void clickConnect(const ProxySettings &proxySettings);
    void clickDisconnect();
    bool isDisconnected();
    void blockingDisconnect();

    //windows specific functions
    QString getConnectedTapAdapter_win();

    void setMss(int mss);

signals:
    void connected();
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(CONNECTION_ERROR errorCode);

private slots:
    void onDnsResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer);

    void onConnectionConnected();
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(CONNECTION_ERROR err);

private:
    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_ERROR_DURING_CONNECTION};

    IHelper *helper_;
    QByteArray ovpnConfig_;
    IConnection *connector_;
    MakeOVPNFile *makeOVPNFile_;
    ProxySettings proxySettings_;

#ifdef Q_OS_MAC
    RestoreDNSManager_mac restoreDnsManager_;
#endif

    struct CONNECT_ATTEMPT_INFO
    {
        QString ip;
        uint port;
        QString protocol;   //udp or tcp
    };
    QVector<CONNECT_ATTEMPT_INFO> attempts_;

    QString lastDefaultGateway_;
    QString lastIp_;
    uint serverApiUserRole_;
    int state_;
    int mss_;

    void doConnect();
    void doMacRestoreProcedures();
    void addRandomHardcodedIpsToAttempts();
};

#endif // EMERGENCYCONTROLLER_H
