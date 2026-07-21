#pragma once

#include <Windows.h>
#include <ws2def.h>

#include "helper_base.h"
#include "types/protocol.h"

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

    void setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts);
    bool sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                           const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const types::Protocol &protocol,
                           const QStringList &dnsWhitelistIps = QStringList());
    void changeMtu(const QString &adapter, int mtu);
    // OpenVPN chooses its own management port; on success outPort/outPid receive the OS-assigned
    // management port and the OpenVPN PID, which the engine uses to verify the management peer.
    bool executeOpenVPN(const QString &config, const QString &httpProxy, unsigned int httpPort,
                        const QString &socksProxy, unsigned int socksPort, unsigned int &outPort, unsigned long &outPid);

    bool executeTaskKill(CmdKillTarget target);

    bool startWireGuard(bool useAmneziaWG);
    bool stopWireGuard();
    bool configureWireGuard(const WireGuardConfig &config);
    bool getWireGuardStatus(types::WireGuardStatus *status);

    void firewallOn(const QString &connectingIp, const QStringList &ips, bool bAllowLanTraffic, bool bIsCustomConfig);
    void firewallOff();
    bool firewallActualState();

    bool setCustomDnsWhileConnected(unsigned long ifIndex, const QString &overrideDnsIpAddress);

    bool executeSetMetric(ADDRESS_FAMILY family, const QString &interfaceName, ULONG metric);
    bool isWanIkev2AdapterDisabled();

    bool isIcsSupported();
    bool startIcs();
    bool changeIcs(const QString &adapterName);
    bool stopIcs();

    unsigned long queryBFEStatus();
    bool enableBFE();
    QString resetAndStartRAS();

    // bAllowLanTraffic/bIsCustomConfig are only meaningful when disabling IPv6 (b == false):
    // they gate the LAN (fc00::/7, ff00::/8) permits in the helper's Ipv6Firewall sublayer.
    void setIPv6EnabledInFirewall(bool b, bool bAllowLanTraffic = false, bool bIsCustomConfig = false);
    void setFirewallOnBoot(bool b);

    bool clearWifiHistoryData();

    bool addHosts(const QString &ip, const QString &hostname);
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
    void resetNetworkAdapter(int ifIndex, bool bringAdapterBackUp);

    void addIKEv2DefaultRoute();
    void removeAppNetworkProfiles();
    void setIKEv2IPSecParameters();
    bool makeHostsFileWritable();

    void setCustomDnsIps(const QStringList& ips);

    void createOpenVpnAdapter(bool useDCODriver);
    void removeOpenVpnAdapter();

    void disableDohSettings();
    void enableDohSettings();

    unsigned long ssidFromInterfaceGUID(const QString &interfaceGUID, QString &ssid);

    QString installerStageAndVerify(const QString &srcPath);
    void installerCleanupStaged();

private:
    QStringList customDnsIp_;
};
