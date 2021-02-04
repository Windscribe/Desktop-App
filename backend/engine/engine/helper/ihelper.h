#ifndef IHELPER_H
#define IHELPER_H

#include <QThread>

class SplitTunnelingNetworkInfo;
class WireGuardConfig;
struct WireGuardStatus;
class AdapterGatewayInfo;
class ProtocolType;

class IHelper : public QThread
{
    Q_OBJECT
public:
    explicit IHelper(QObject *parent = 0) : QThread(parent) {}
    virtual ~IHelper() {}

    virtual void startInstallHelper() = 0;
    virtual QString executeRootCommand(const QString &commandLine) = 0;
    virtual bool executeRootUnblockingCommand(const QString &commandLine, unsigned long &outCmdId, const QString &eventName) = 0;
    virtual bool executeOpenVPN(const QString &commandLine, const QString &pathToOvpnConfig, unsigned long &outCmdId) = 0;
    virtual bool executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort,
                                const QString &socksProxy, unsigned int socksPort,
                                unsigned long &outCmdId) = 0;
    virtual bool executeTaskKill(const QString &executableName) = 0;
    virtual bool executeResetTap(const QString &tapName) = 0;
    virtual QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber) = 0;
    virtual QString executeWmicEnable(const QString &adapterName) = 0;
    virtual QString executeWmicGetConfigManagerErrorCode(const QString &adapterName) = 0;
    virtual bool executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid,
                                     unsigned long &outCmdId, const QString &eventName) = 0;
    virtual bool executeChangeMtu(const QString &adapter, int mtu) = 0;

    virtual QString executeUpdateInstaller(const QString &installerPath, bool &success) = 0;

    virtual bool clearDnsOnTap() = 0;
    virtual QString enableBFE() = 0;
    virtual QString resetAndStartRAS() = 0;


    virtual void setIPv6EnabledInFirewall(bool b) = 0;
    virtual void setIPv6EnabledInOS(bool b) = 0;
    virtual bool IPv6StateInOS() = 0;

    virtual bool isHelperConnected() = 0;
    virtual bool isFailedConnectToHelper() = 0;

    virtual void setNeedFinish() = 0;

    virtual QString getHelperVersion() = 0;

    virtual void enableMacSpoofingOnBoot(bool bEnable, QString interfaceName, QString macAddress) = 0;
    virtual void enableFirewallOnBoot(bool bEnable) = 0;

    virtual bool removeWindscribeUrlsFromHosts() = 0;
    virtual bool addHosts(const QString &hosts) = 0;
    virtual bool removeHosts() = 0;

    virtual bool reinstallHelper() = 0;

    virtual void closeAllTcpConnections(bool isKeepLocalSockets) = 0;
    virtual QStringList getProcessesList() = 0;

    virtual bool whitelistPorts(const QString &ports) = 0;
    virtual bool deleteWhitelistPorts() = 0;

    virtual bool setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts) = 0;

    virtual void sendConnectStatus(bool isConnected, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const ProtocolType &protocol) = 0;

    // windows specific functions
    virtual void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished) = 0;
    virtual void clearUnblockingCmd(unsigned long cmdId) = 0;
    virtual void suspendUnblockingCmd(unsigned long cmdId) = 0;
    virtual void enableDnsLeaksProtection() = 0;
    virtual void disableDnsLeaksProtection() = 0;
    virtual bool reinstallWanIkev2() = 0;
    virtual bool enableWanIkev2() = 0;
    virtual bool setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value) = 0;
    virtual bool removeMacAddressRegistryProperty(QString subkeyInterfaceName) = 0;
    virtual bool resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp) = 0;
    virtual bool addIKEv2DefaultRoute() = 0;
    virtual bool removeWindscribeNetworkProfiles() = 0;
    virtual void setIKEv2IPSecParameters() = 0;

#ifdef Q_OS_WIN
    virtual bool isSupportedICS() = 0;
#endif

    // mac specific functions
    virtual QStringList getActiveNetworkInterfaces_mac() = 0;
    virtual bool setKeychainUsernamePassword(const QString &username, const QString &password) = 0;
    virtual bool setKextPath(const QString &kextPath) = 0;

    // WireGuard functions
    virtual bool startWireGuard(const QString &exeName, const QString &deviceName) = 0;
    virtual bool stopWireGuard() = 0;
    virtual bool configureWireGuard(const WireGuardConfig &config) = 0;
    virtual bool getWireGuardStatus(WireGuardStatus *status) = 0;

signals:
    void lostConnectionToHelper();
};

#endif // IHELPER_H
