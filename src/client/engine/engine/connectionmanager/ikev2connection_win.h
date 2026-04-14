#pragma once

#include <windows.h>
#include <ras.h>
#include <raserror.h>

#include <QMap>
#include <QMutex>
#include <QString>
#include <QTimer>

#include "IConnection.h"
#include "engine/helper/helper.h"
#include "ikev2connectiondisconnectlogic_win.h"


class IKEv2Connection_win : public IConnection
{
    Q_OBJECT
public:
    explicit IKEv2Connection_win(QObject *parent, Helper *helper);
    ~IKEv2Connection_win() override;

    void startConnect(const QString &configPathOrUrl, const QString &ip, const QString &dnsHostName,  const QString &username, const QString &password, const types::ProxySettings &proxySettings,
                      const WireGuardConfig *wireGuardConfig, bool isEnableIkev2Compression, bool isCustomConfig, const QString &overrideDnsIp) override;
    void startDisconnect() override;
    bool isDisconnected() const override;
    void waitForDisconnect() override;

    //QString getConnectedTapTunAdapterName() override;
    ConnectionType getConnectionType() const override { return ConnectionType::IKEV2; }

    static void removeIkev2ConnectionFromOS();
    static QVector<HRASCONN> getActiveAppConnections();

    void continueWithUsernameAndPassword(const QString &username, const QString &password) override;
    void continueWithPassword(const QString &password) override;

private slots:
    void onTimerControlConnection();
    void handleAuthError();
    void handleErrorReinstallWan();
    void onHandleDisconnectLogic();

private:
    enum {
        STATE_DISCONNECTED,
        STATE_CONNECTED,
        STATE_CONNECTING,
        STATE_DISCONNECTING,
        STATE_REINSTALL_WAN
    };

    int state_ = STATE_DISCONNECTED;
    Helper *helper_ = NULL;

    QString initialUrl_;
    QString initialIp_;
    QString initialUsername_;
    QString initialPassword_;
    bool initialEnableIkev2Compression_ = false;

    HRASCONN connHandle_ = NULL;
    QTimer timerControlConnection_;
    static constexpr int CONTROL_TIMER_PERIOD = 1000;
    QMap<RASCONNSTATE, QString> mapConnStates_;

    mutable QRecursiveMutex mutex_;

    IKEv2ConnectionDisconnectLogic_win disconnectLogic_;

    int cntFailedConnectionAttempts_ = 0;
    static constexpr int MAX_FAILED_CONNECTION_ATTEMPTS = 3;

    void doConnect();
    void rasDialFuncCallback(HRASCONN hrasconn, UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);

    void doBlockingDisconnect();

    bool getIKEv2Device(RASDEVINFO *outDevInfo);
    void initMapConnStates();
    QString rasConnStateToString(RASCONNSTATE state);

    static bool wanReinstalled_;
    static DWORD CALLBACK RasDial2CallbackFunc(ULONG_PTR dwCallbackId, DWORD unused, HRASCONN hrasconn, UINT unMsg, RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
};
