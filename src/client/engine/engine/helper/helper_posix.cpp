#include "helper_posix.h"
#include "convert_utils.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "utils/ws_assert.h"

#ifdef Q_OS_LINUX
#include "utils/dnsscripts_linux.h"
#endif

using namespace convert_utils;

Helper_posix::Helper_posix(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger) : HelperBase(std::move(backend), logger)
{
}

QString Helper_posix::getHelperVersion()
{
    auto result = sendCommand(HelperCommand::getHelperVersion);
    std::string version;
    deserializeAnswer(result, version);
    return QString::fromStdString(version);
}

void Helper_posix::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    auto result = sendCommand(HelperCommand::getUnblockingCmdStatus, cmdId);
    std::string log;
    outFinished = false;
    deserializeAnswer(result, outFinished, log);
    outLog = QString::fromStdString(log);
}

void Helper_posix::clearUnblockingCmd(unsigned long cmdId)
{
    sendCommand(HelperCommand::clearUnblockingCmd, cmdId);
}

void Helper_posix::suspendUnblockingCmd(unsigned long cmdId)
{
    // On posix, this is the same as clearing a cmd.
    clearUnblockingCmd(cmdId);
}

void Helper_posix::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isAllowLanTraffic, const QStringList &files, const QStringList &ips, const QStringList &hosts)
{
    sendCommand(HelperCommand::setSplitTunnelingSettings, isActive, isExclude, isAllowLanTraffic,
                toStdVector(files), toStdVector(ips), toStdVector(hosts));
}

bool Helper_posix::sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter, const QString &connectedIp, const types::Protocol &protocol)
{
    auto fillAdapterInfo = [](const AdapterGatewayInfo &a, ADAPTER_GATEWAY_INFO &out) {
        out.adapterName = a.adapterName().toStdString();
        out.adapterIp = a.adapterIp().toStdString();
        out.gatewayIp = a.gateway().toStdString();
        out.ifIndex = a.ifIndex();
        const QStringList dns = a.dnsServers();
        for(const auto &ip : dns) {
            out.dnsServers.push_back(ip.toStdString());
        }
    };

    ADAPTER_GATEWAY_INFO defaultAdapterInfo, vpnAdapterInfo;
    CmdProtocolType cmdProtocol = kCmdProtocolWireGuard;
    if (isConnected) {
        if (protocol.isStunnelOrWStunnelProtocol())
            cmdProtocol = kCmdProtocolStunnelOrWstunnel;
        else if (protocol.isIkev2Protocol())
            cmdProtocol = kCmdProtocolIkev2;
        else if (protocol.isWireGuardProtocol())
            cmdProtocol = kCmdProtocolWireGuard;
        else if (protocol.isOpenVpnProtocol())
            cmdProtocol = kCmdProtocolOpenvpn;
        else
            WS_ASSERT(false);

        fillAdapterInfo(vpnAdapter, vpnAdapterInfo);
    }

    fillAdapterInfo(defaultAdapter, defaultAdapterInfo);
    auto result = sendCommand(HelperCommand::sendConnectStatus, isConnected, cmdProtocol, defaultAdapterInfo, vpnAdapterInfo,
                connectedIp.toStdString(), vpnAdapter.remoteIp().toStdString());
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

void Helper_posix::changeMtu(const QString &adapter, int mtu)
{
    sendCommand(HelperCommand::changeMtu, adapter.toStdString(), mtu);
}

bool Helper_posix::executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort, bool isCustomConfig, unsigned long &outCmdId)
{
    CmdDnsManager dnsManager;
#if defined(Q_OS_LINUX)
    switch (DnsScripts_linux::instance().dnsManager()) {
    case DnsScripts_linux::SCRIPT_TYPE::SYSTEMD_RESOLVED:
        dnsManager = kSystemdResolved;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::RESOLV_CONF:
        dnsManager = kResolvConf;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::NETWORK_MANAGER:
    default:
        dnsManager = kNetworkManager;
        break;
    }
#endif
    auto result = sendCommand(HelperCommand::executeOpenVPN, config.toStdString(), port, httpProxy.toStdString(), httpPort,
                              socksProxy.toStdString(), socksPort, isCustomConfig, dnsManager);
    bool success = false;
    deserializeAnswer(result, success, outCmdId);
    return success;
}

bool Helper_posix::executeTaskKill(CmdKillTarget target)
{
    auto result = sendCommand(HelperCommand::executeTaskKill, target);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::startCtrld(const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog)
{
    auto result = sendCommand(HelperCommand::startCtrld, upstream1.toStdString(), upstream2.toStdString(), toStdVector(domains), isCreateLog);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::stopCtrld()
{
    return executeTaskKill(kTargetCtrld);
}

bool Helper_posix::checkFirewallState(const QString &tag)
{
    auto result = sendCommand(HelperCommand::checkFirewallState, tag.toStdString());
    bool isEnabled = false;
    deserializeAnswer(result, isEnabled);
    return isEnabled;
}

bool Helper_posix::clearFirewallRules(bool isKeepPfEnabled)
{
    sendCommand(HelperCommand::clearFirewallRules, isKeepPfEnabled);
    return true;
}

bool Helper_posix::setFirewallRules(CmdIpVersion version, const QString &table, const QString &group, const QString &rules)
{
    sendCommand(HelperCommand::setFirewallRules, version, table.toStdString(), group.toStdString(), rules.toStdString());
    return true;
}

bool Helper_posix::getFirewallRules(CmdIpVersion version, const QString &table, const QString &group, QString &rules)
{
    auto result = sendCommand(HelperCommand::getFirewallRules, version, table.toStdString(), group.toStdString());
    std::string tempRules;
    deserializeAnswer(result, tempRules);
    rules = QString::fromStdString(tempRules);
    return true;
}

bool Helper_posix::setFirewallOnBoot(bool enabled, const QSet<QString> &ipTable, bool allowLanTraffic)
{
    std::string ipTableStr = "";
    for (const auto& ip : ipTable) {
        ipTableStr += ip.toStdString();
        ipTableStr += " ";
    }
    sendCommand(HelperCommand::setFirewallOnBoot, enabled, allowLanTraffic, ipTableStr);
    return true;
}

bool Helper_posix::startStunnel(const QString &hostname, unsigned int port, unsigned int localPort, bool extraPadding)
{
    auto result = sendCommand(HelperCommand::startStunnel, hostname.toStdString(), port, localPort, extraPadding);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::startWstunnel(const QString &hostname, unsigned int port, unsigned int localPort)
{
    auto result = sendCommand(HelperCommand::startWstunnel, hostname.toStdString(), port, localPort);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::setMacAddress(const QString &interface, const QString &macAddress, const QString &network, bool isWifi)
{
    auto result = sendCommand(HelperCommand::setMacAddress, interface.toStdString(), macAddress.toStdString(), network.toStdString(), isWifi);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

