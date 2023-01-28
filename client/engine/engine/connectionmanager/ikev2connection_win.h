#ifndef IKEV2CONNECTION_WIN_H
#define IKEV2CONNECTION_WIN_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QRecursiveMutex>
#include "Engine/Types/types.h"
#include "IConnection.h"
#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include "engine/helper/helper_win.h"
#include "ikev2connectiondisconnectlogic_win.h"


class IKEv2Connection_win : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_win(QObject *parent, IHelper *helper);
    ~IKEv2Connection_win() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,  const QString &username, const QString &password, const ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isAutomaticConnectionMode) override;
    void startDisconnect() override;
    bool isDisconnected() const override;

    //QString getConnectedTapTunAdapterName() override;
    ConnectionType getConnectionType() const override { return ConnectionType::IKEV2; }

    static void removeIkev2ConnectionFromOS();
    static QVector<HRASCONN> getActiveWindscribeConnections();

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;

private slots:
    void onTimerControlConnection();
    void handleAuthError();
    void handleErrorReinstallWan();
    void onHandleDisconnectLogic();

private:
    enum { STATE_DISCONNECTED,
           STATE_CONNECTED,
           STATE_CONNECTING,
           STATE_DISCONNECTING,
           STATE_REINSTALL_WAN
         };

    int state_;
    Helper_win *helper_;

    QString initialUrl_;
    QString initialIp_;
    QString initialUsername_;
    QString initialPassword_;
    bool initialEnableIkev2Compression_;
    bool isAutomaticConnectionMode_;

    HRASCONN connHandle_;
    QTimer timerControlConnection_;
    static constexpr int CONTROL_TIMER_PERIOD = 1000;
    QMap<RASCONNSTATE, QString> mapConnStates_;

    mutable QRecursiveMutex mutex_;

    IKEv2ConnectionDisconnectLogic_win disconnectLogic_;

    int cntFailedConnectionAttempts_;
    static constexpr int MAX_FAILED_CONNECTION_ATTEMPTS = 4;
    static constexpr int MAX_FAILED_CONNECTION_ATTEMPTS_FOR_AUTOMATIC_MODE = 3;

    void doConnect();
    void rasDialFuncCallback(HRASCONN hrasconn, UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);

    void doBlockingDisconnect();

    bool getIKEv2Device(RASDEVINFO *outDevInfo);
    void initMapConnStates();
    QString rasConnStateToString(RASCONNSTATE state);

    static IKEv2Connection_win *this_;
    static bool wanReinstalled_;
    static void CALLBACK staticRasDialFunc(HRASCONN hrasconn, UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
};

#endif // IKEV2CONNECTION_WIN_H
