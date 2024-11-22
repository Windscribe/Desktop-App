#pragma once

#include <QMutex>
#include <QTimer>
#include <qt_windows.h>

#include <atomic>

#include "ihelper.h"
#include "../../../../backend/windows/windscribe_service/ipc/servicecommunication.h"
#include "utils/servicecontrolmanager.h"
#include "utils/win32handle.h"

class Helper_win : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_win(QObject *parent = NULL);
    ~Helper_win() override;

    void startInstallHelper() override;
    STATE currentState() const override;
    bool reinstallHelper() override;
    void setNeedFinish() override;
    QString getHelperVersion() override;

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) override;
    void clearUnblockingCmd(unsigned long cmdId) override;
    void suspendUnblockingCmd(unsigned long cmdId) override;

    bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts) override;
    bool sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                           const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const types::Protocol &protocol) override;
    bool setCustomDnsWhileConnected(unsigned long ifIndex, const QString &overrideDnsIpAddress);
    bool changeMtu(const QString &adapter, int mtu) override;
    bool executeTaskKill(CmdKillTarget target);
    ExecuteError executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                                const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId, bool isCustomConfig) override;

    // WireGuard functions
    IHelper::ExecuteError startWireGuard() override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(types::WireGuardStatus *status) override;

    // ctrld functions
    bool startCtrld(const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog) override;
    bool stopCtrld() override;

    // Windows specific functions
    bool isHelperConnected() const;
    QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber);
    QString executeWmicGetConfigManagerErrorCode(const QString &adapterName);

    bool isIcsSupported();
    bool startIcs();
    bool changeIcs(const QString &adapterName);
    bool stopIcs();

    QString enableBFE();
    QString resetAndStartRAS();

    bool addHosts(const QString &hosts);
    bool removeHosts();

    void closeAllTcpConnections(bool isKeepLocalSockets);
    QStringList getProcessesList();

    bool whitelistPorts(const QString &ports);
    bool deleteWhitelistPorts();

    void enableDnsLeaksProtection();
    void disableDnsLeaksProtection();

    bool reinstallWanIkev2();
    bool enableWanIkev2();

    bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value);
    bool removeMacAddressRegistryProperty(QString subkeyInterfaceName);
    bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp);

    bool addIKEv2DefaultRoute();
    bool removeWindscribeNetworkProfiles();
    void setIKEv2IPSecParameters();
    bool makeHostsFileWritable();

    void setCustomDnsIps(const QStringList& ips);

    bool createOpenVpnAdapter(bool useDCODriver);
    bool removeOpenVpnAdapter();

    bool disableDohSettings();
    bool enableDohSettings();

    DWORD ssidFromInterfaceGUID(const QString &interfaceGUID, QString &ssid);

protected:
    void run() override;

private:
    enum {MAX_WAIT_TIME_FOR_PIPE = 10000};
    enum {CHECK_UNBLOCKING_CMD_PERIOD = 2000};

    QStringList customDnsIp_;
    std::atomic<STATE> curState_;

    QMutex mutex_;
    wsl::ServiceControlManager scm_;
    wsl::Win32Handle helperPipe_;

    MessagePacketResult sendCmdToHelper(int cmdId, const std::string &data);
    bool disableIPv6();
    bool enableIPv6();

    int debugGetActiveUnblockingCmdCount();

    friend class FirewallController_win;
    // friend functions for windows firewall controller class
    bool firewallOn(const QString &connectingIp, const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig);
    bool firewallOff();
    bool firewallActualState();
    void initVariables();

    bool readAllFromPipe(HANDLE hPipe, char *buf, DWORD len);
    bool writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len);
};
