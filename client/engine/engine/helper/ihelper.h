#pragma once

#include <QThread>
#include "types/protocol.h"

class SplitTunnelingNetworkInfo;
class WireGuardConfig;
class AdapterGatewayInfo;

namespace types
{
    struct WireGuardStatus;
}

// basic helper class for execute root commands
// universal functions for all platforms are declared here
class IHelper : public QThread
{
    Q_OBJECT
public:
    enum STATE { STATE_INIT, STATE_CONNECTED, STATE_FAILED_CONNECT, STATE_USER_CANCELED, STATE_INSTALL_FAILED };

    explicit IHelper(QObject *parent = 0) : QThread(parent) {}
    virtual ~IHelper() {}

    enum ExecuteError { EXECUTE_SUCCESS, EXECUTE_ERROR };

    virtual void startInstallHelper() = 0;
    virtual STATE currentState() const = 0;
    virtual bool reinstallHelper() = 0;
    virtual void setNeedFinish() = 0;
    virtual QString getHelperVersion() = 0;

    virtual void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) = 0;
    virtual void clearUnblockingCmd(unsigned long cmdId) = 0;
    virtual void suspendUnblockingCmd(unsigned long cmdId) = 0;

    virtual bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts) = 0;
    virtual bool sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                                   const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const types::Protocol &protocol) = 0;
    virtual bool changeMtu(const QString &adapter, int mtu) = 0;

    virtual ExecuteError executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                                        const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId, bool isCustomConfig) = 0;

    // WireGuard functions
    virtual ExecuteError startWireGuard() = 0;
    virtual bool stopWireGuard() = 0;
    virtual bool configureWireGuard(const WireGuardConfig &config) = 0;
    virtual bool getWireGuardStatus(types::WireGuardStatus *status) = 0;

    // ctrld functions
    virtual ExecuteError startCtrld(const QString &ip, const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog) = 0;
    virtual bool stopCtrld() = 0;

signals:
    void lostConnectionToHelper();
};
