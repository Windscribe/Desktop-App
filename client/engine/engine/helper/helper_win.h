#pragma once

#include "helper_base.h"
#include "types/protocol.h"
#include "types/enums.h"

class AdapterGatewayInfo;
class WireGuardConfig;
namespace types
{
    struct WireGuardStatus;
}

// Windows commands
class Helper_win : public HelperBase
{
public:
    explicit Helper_win(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger);
    virtual ~Helper_win() {}

    QString getHelperVersion();

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished);
    void clearUnblockingCmd(unsigned long cmdId);
    void suspendUnblockingCmd(unsigned long cmdId);

    void setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts);
    bool sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                           const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const types::Protocol &protocol);
    void changeMtu(const QString &adapter, int mtu);
    bool executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                                const QString &socksProxy, unsigned int socksPort, bool isCustomConfig, unsigned long &outCmdId);

    bool executeTaskKill(CmdKillTarget target);

    bool startWireGuard();
    bool stopWireGuard();
    bool configureWireGuard(const WireGuardConfig &config);
    bool getWireGuardStatus(types::WireGuardStatus *status);

    void firewallOn(const QString &connectingIp, const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig);
    void firewallOff();
    bool firewallActualState();

    bool setCustomDnsWhileConnected(unsigned long ifIndex, const QString &overrideDnsIpAddress);

    QString executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber);
    QString executeWmicGetConfigManagerErrorCode(const QString &adapterName);

    bool isIcsSupported();
    bool startIcs();
    bool changeIcs(const QString &adapterName);
    bool stopIcs();

    QString enableBFE();
    QString resetAndStartRAS();

    void setIPv6EnabledInFirewall(bool b);
    void setFirewallOnBoot(bool b);

    bool addHosts(const QString &hosts);
    bool removeHosts();

    void closeAllTcpConnections(bool isKeepLocalSockets);
    QStringList getProcessesList();

    bool whitelistPorts(const QString &ports);
    bool deleteWhitelistPorts();

    void enableDnsLeaksProtection();
    void disableDnsLeaksProtection();

    void reinstallWanIkev2();
    void enableWanIkev2();

    void setMacAddressRegistryValueSz(const QString &subkeyInterfaceName, const QString &value);
    void removeMacAddressRegistryProperty(const QString &subkeyInterfaceName);
    void resetNetworkAdapter(const QString &subkeyInterfaceName, bool bringAdapterBackUp);

    void addIKEv2DefaultRoute();
    void removeWindscribeNetworkProfiles();
    void setIKEv2IPSecParameters();
    bool makeHostsFileWritable();

    void setCustomDnsIps(const QStringList& ips);

    void createOpenVpnAdapter(bool useDCODriver);
    void removeOpenVpnAdapter();

    void disableDohSettings();
    void enableDohSettings();

    unsigned long ssidFromInterfaceGUID(const QString &interfaceGUID, QString &ssid);

    void setNetworkCategory(const QString &networkName, NETWORK_CATEGORY category);

private:
    QStringList customDnsIp_;
};
