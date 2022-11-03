#ifndef HELPER_WIN_H
#define HELPER_WIN_H

#include <QMutex>
#include <QTimer>
#include <qt_windows.h>

#include <atomic>

#include "ihelper.h"
#include "../../../../backend/windows/windscribe_service/ipc/servicecommunication.h"

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
    bool setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress) override;

     // WireGuard functions
    IHelper::ExecuteError startWireGuard(const QString &exeName, const QString &deviceName) override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(types::WireGuardStatus *status) override;
    void setDefaultWireGuardDeviceName(const QString &deviceName) override;

    // Windows specific functions
    bool isHelperConnected() const;
    IHelper::ExecuteError executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort,
                        const QString &socksProxy, unsigned int socksPort,
                        unsigned long &outCmdId);
    bool executeTaskKill(const QString &executableName);
    bool executeResetTap(const QString &tapName);
    QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber);
    QString executeWmicEnable(const QString &adapterName);
    QString executeWmicGetConfigManagerErrorCode(const QString &adapterName);
    bool executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid,
                          unsigned long &outCmdId, const QString &eventName);
    bool executeChangeMtu(const QString &adapter, int mtu);

    bool clearDnsOnTap();
    QString enableBFE();
    QString resetAndStartRAS();

    void setIPv6EnabledInFirewall(bool b);
    void setIPv6EnabledInOS(bool b);
    bool IPv6StateInOS();

    bool addHosts(const QString &hosts);
    bool removeHosts();

    void closeAllTcpConnections(bool isKeepLocalSockets);
    QStringList getProcessesList();

    bool whitelistPorts(const QString &ports);
    bool deleteWhitelistPorts();

    bool isSupportedICS();

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
    bool reinstallTapDriver(const QString& tapDriverDir);
    bool reinstallWintunDriver(const QString& wintunDriverDir);

    void setCustomDnsIp(const QString& ip);

protected:
    void run() override;

private:
    enum {MAX_WAIT_TIME_FOR_HELPER = 30000};
    enum {MAX_WAIT_TIME_FOR_PIPE = 10000};
    enum {CHECK_UNBLOCKING_CMD_PERIOD = 2000};

    QString helperLabel_;
    QString customDnsIp_;
    std::atomic<STATE> curState_;
    std::atomic<bool> bStopThread_;

    bool bIPV6State_;
    QMutex mutex_;

    MessagePacketResult sendCmdToHelper(int cmdId, const std::string &data);
    bool disableIPv6();
    bool enableIPv6();

    bool disableIPv6InOS();
    bool enableIPv6InOS();


    int debugGetActiveUnblockingCmdCount();

    friend class FirewallController_win;
    // friend functions for windows firewall controller class
    bool firewallOn(const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig);
    bool firewallChange(const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig);
    bool firewallOff();
    bool firewallActualState();
    void initVariables();

    bool readAllFromPipe(HANDLE hPipe, char *buf, DWORD len);
    bool writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len);
};

#endif // HELPER_WIN_H
