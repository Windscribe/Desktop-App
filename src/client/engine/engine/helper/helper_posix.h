#pragma once

#include "helper_base.h"
#include "types/protocol.h"
#include "../../helper/common/helper_commands.h"

class AdapterGatewayInfo;
class WireGuardConfig;
namespace types
{
struct WireGuardStatus;
}

// Posix (Mac and Linux) commands
class Helper_posix : public HelperBase
{
public:
    explicit Helper_posix(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger);

    QString getHelperVersion();

    void getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished);
    void clearUnblockingCmd(unsigned long cmdId);
    void suspendUnblockingCmd(unsigned long cmdId);

    void setSplitTunnelingSettings(bool isActive, bool isExclude, bool isAllowLanTraffic,
                                   const QStringList &files, const QStringList &ips,
                                   const QStringList &hosts);
    bool sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                           const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                           const QString &connectedIp, const types::Protocol &protocol);
    void changeMtu(const QString &adapter, int mtu);
    bool executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                        const QString &socksProxy, unsigned int socksPort, bool isCustomConfig, unsigned long &outCmdId);

    bool executeTaskKill(CmdKillTarget target);

    bool startCtrld(const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog);
    bool stopCtrld();

    bool checkFirewallState(const QString &tag);
    bool clearFirewallRules(bool isKeepPfEnabled);
    bool setFirewallRules(CmdIpVersion version, const QString &table, const QString &group, const QString &rules);
    bool getFirewallRules(CmdIpVersion version, const QString &table, const QString &group, QString &rules);
    bool setFirewallOnBoot(bool enabled, const QSet<QString>& ipTable, bool allowLanTraffic);
    bool startStunnel(const QString &hostname, unsigned int port, unsigned int localPort, bool extraPadding);
    bool startWstunnel(const QString &hostname, unsigned int port, unsigned int localPort);
    bool setMacAddress(const QString &interface, const QString &macAddress, const QString &network = "", bool isWifi = false);
};
