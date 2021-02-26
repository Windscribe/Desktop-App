#ifndef HELPER_WIN_H
#define HELPER_WIN_H

#include "ihelper.h"
#include <QTimer>
#include <QMutex>
#include <atomic>
#include <Windows.h>
#include "../../../windows/windscribe_service/ipc/servicecommunication.h"
#include "../../../windows/windscribe_service/ipc/serialize_structs.h"

class Helper_win : public IHelper
{
    Q_OBJECT
public:
    explicit Helper_win(QObject *parent = NULL);
    ~Helper_win() override;

    void startInstallHelper() override;
    QString executeRootCommand(const QString &commandLine) override;
    bool executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName) override;
    bool executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId) override;
    bool executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort,
                        const QString &socksProxy, unsigned int socksPort,
                        unsigned long &outCmdId) override;
    bool executeTaskKill(const QString &executableName) override;
    bool executeResetTap(const QString &tapName) override;
    QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber) override;
    QString executeWmicEnable(const QString &adapterName) override;
    QString executeWmicGetConfigManagerErrorCode(const QString &adapterName) override;
    bool executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid,
                          unsigned long &outCmdId, const QString &eventName) override;
    bool executeChangeMtu(const QString &adapter, int mtu) override;

    QString executeUpdateInstaller(const QString &installerPath, bool &success) override;

    bool clearDnsOnTap() override;
    QString enableBFE() override;
    QString resetAndStartRAS() override;

    void setIPv6EnabledInFirewall(bool b) override;
    void setIPv6EnabledInOS(bool b) override;
    bool IPv6StateInOS() override;

    bool isHelperConnected() override;
    bool isFailedConnectToHelper() override;

    void setNeedFinish() override;
    QString getHelperVersion() override;

    void enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress) override;
    void enableFirewallOnBoot(bool bEnable) override;

    bool removeWindscribeUrlsFromHosts() override;
    bool addHosts(const QString &hosts) override;
    bool removeHosts() override;

    bool reinstallHelper() override;

    void closeAllTcpConnections(bool isKeepLocalSockets) override;
    QStringList getProcessesList() override;

    bool whitelistPorts(const QString &ports) override;
    bool deleteWhitelistPorts() override;

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) override;
    void clearUnblockingCmd(unsigned long cmdId) override;
    void suspendUnblockingCmd(unsigned long cmdId) override;

    bool isSupportedICS() override;

    QStringList getActiveNetworkInterfaces_mac() override;
    bool setKeychainUsernamePassword(const QString &username, const QString &password) override;

    void enableDnsLeaksProtection() override;
    void disableDnsLeaksProtection() override;

    bool reinstallWanIkev2() override;
    bool enableWanIkev2() override;

    bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value) override;
    bool removeMacAddressRegistryProperty(QString subkeyInterfaceName) override;
    bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp) override;

    bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts) override;

    void sendConnectStatus(bool isConnected, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const ProtocolType &protocol) override;


    bool addIKEv2DefaultRoute() override;
    bool removeWindscribeNetworkProfiles() override;
    void setIKEv2IPSecParameters() override;
    bool setKextPath(const QString &kextPath) override;

    bool startWireGuard(const QString &exeName, const QString &deviceName) override;
    bool stopWireGuard() override;
    bool configureWireGuard(const WireGuardConfig &config) override;
    bool getWireGuardStatus(WireGuardStatus *status) override;
    void setDefaultWireGuardDeviceName(const QString &deviceName) override;

protected:
    void run() override;

private:
    enum {MAX_WAIT_TIME_FOR_HELPER = 30000};
    enum {MAX_WAIT_TIME_FOR_PIPE = 10000};
    enum {CHECK_UNBLOCKING_CMD_PERIOD = 2000};

    QString helperLabel_;
    std::atomic<bool> bFailedConnectToHelper_;
    std::atomic<bool> bHelperConnected_;
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
    bool firewallOn(const QString &ip, bool bAllowLanTraffic);
    bool firewallChange(const QString &ip, bool bAllowLanTraffic);
    bool firewallOff();
    bool firewallActualState();
    void initVariables();

    bool readAllFromPipe(HANDLE hPipe, char *buf, DWORD len);
    bool writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len);
};

#endif // HELPER_WIN_H
