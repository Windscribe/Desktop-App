#include "helper_win.h"
#include <basetsd.h>
#include "utils/log/categories.h"
#include "convert_utils.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "../../backend/common/helper_commands.h"
#include "utils/winutils.h"
#include "utils/ws_assert.h"

using namespace convert_utils;

Helper_win::Helper_win(std::unique_ptr<IHelperBackend> backend) : HelperBase(std::move(backend))
{
}

QString Helper_win::getHelperVersion()
{
    QString version = WinUtils::getVersionInfoItem(QCoreApplication::applicationDirPath() + "/WindscribeService.exe", "ProductVersion");
    if (version.isEmpty()) {
        version = "Failed to extract ProductVersion from executable";
    }
    return version;
}

//TODO: check that works with openvpn
void Helper_win::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    auto result = sendCommand(HelperCommand::getUnblockingCmdStatus, cmdId);
    universal_string log;
    deserializeAnswer(result, outFinished, log);
    outLog = toQString(log);
}

void Helper_win::clearUnblockingCmd(unsigned long cmdId)
{
    sendCommand(HelperCommand::clearUnblockingCmd, cmdId);
}

void Helper_win::suspendUnblockingCmd(unsigned long cmdId)
{
    sendCommand(HelperCommand::suspendUnblockingCmd, cmdId);
}

void Helper_win::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isAllowLanTraffic,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts)
{
    sendCommand(HelperCommand::setSplitTunnelingSettings, isActive, isExclude, isAllowLanTraffic,
                toStdVector(files), toStdVector(ips), toStdVector(hosts));
}

bool Helper_win::sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                               const QString &connectedIp, const types::Protocol &protocol)
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
    CmdProtocolType cmdProtocol;
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
    sendCommand(HelperCommand::sendConnectStatus, isConnected, isTerminateSocket, isKeepLocalSocket, cmdProtocol, defaultAdapterInfo, vpnAdapterInfo,
                connectedIp.toStdString(), vpnAdapter.remoteIp().toStdString());

    return true;
}

void Helper_win::changeMtu(const QString &adapter, int mtu)
{
    sendCommand(HelperCommand::changeMtu, toStdString(adapter), mtu);
}

bool Helper_win::executeTaskKill(CmdKillTarget target)
{
    auto result = sendCommand(HelperCommand::executeTaskKill, (int)target);
    bool success;
    unsigned long exitCode;
    universal_string log;
    deserializeAnswer(result, success, exitCode, log);

#ifdef Q_OS_WIN
    if (success && exitCode != 0) {
        QString result = toQString(log).trimmed();
        if (!result.startsWith("ERROR: The process") && !result.endsWith("not found.")) {
            qCWarning(LOG_BASIC) << QString("executeTaskKill(%1) failed: %2").arg(target).arg(result);
        }
    }
#endif
    return success;
}

bool Helper_win::executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                            const QString &socksProxy, unsigned int socksPort, bool isCustomConfig, unsigned long &outCmdId)
{
    auto result = sendCommand(HelperCommand::executeOpenVPN, config.toStdWString(), port, httpProxy.toStdWString(), httpPort,
                              socksProxy.toStdWString(), socksPort, isCustomConfig);
    bool success;
    deserializeAnswer(result, success, outCmdId);
    return success;
}

bool Helper_win::startWireGuard()
{
    auto result = sendCommand(HelperCommand::startWireGuard);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::stopWireGuard()
{
    auto result = sendCommand(HelperCommand::stopWireGuard);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::configureWireGuard(const WireGuardConfig &config)
{
    auto result = sendCommand(HelperCommand::configureWireGuard, config.generateConfigFile().toStdWString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::getWireGuardStatus(types::WireGuardStatus *status)
{
    auto result = sendCommand(HelperCommand::getWireGuardStatus);
    UINT64 lastHandshake, txBytes, rxBytes;
    bool success;
    deserializeAnswer(result, success, lastHandshake, txBytes, rxBytes);
    if (!success) {
        status->state = types::WireGuardState::FAILURE;
        status->lastHandshake = 0;
        status->bytesReceived = 0;
        status->bytesTransmitted = 0;
    }
    else {
        status->state = types::WireGuardState::ACTIVE;
        status->lastHandshake = lastHandshake;
        status->bytesTransmitted = txBytes;
        status->bytesReceived = rxBytes;
    }
    return true;
}

void Helper_win::firewallOn(const QString &connectingIp, const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    sendCommand(HelperCommand::firewallOn, connectingIp.toStdWString(), ip.toStdWString(), bAllowLanTraffic, bIsCustomConfig);
}

void Helper_win::firewallOff()
{
    sendCommand(HelperCommand::firewallOff);
}

bool Helper_win::firewallActualState()
{
    auto result = sendCommand(HelperCommand::firewallActualState);
    bool state;
    deserializeAnswer(result, state);
    return state;
}

bool Helper_win::setCustomDnsWhileConnected(unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    auto result = sendCommand(HelperCommand::setCustomDnsWhileConnected, ifIndex, overrideDnsIpAddress.toStdWString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

QString Helper_win::executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber)
{
    auto result = sendCommand(HelperCommand::executeSetMetric, interfaceType.toStdWString(), interfaceName.toStdWString(), metricNumber.toStdWString());
    std::wstring output;
    deserializeAnswer(result, output);
    return QString::fromStdWString(output);
}

QString Helper_win::executeWmicGetConfigManagerErrorCode(const QString &adapterName)
{
    auto result = sendCommand(HelperCommand::executeWmicGetConfigManagerErrorCode, adapterName.toStdWString());
    std::wstring output;
    deserializeAnswer(result, output);
    return QString::fromStdWString(output);
}

bool Helper_win::isIcsSupported()
{
    auto result = sendCommand(HelperCommand::isIcsSupported);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::startIcs()
{
    auto result = sendCommand(HelperCommand::startIcs);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::changeIcs(const QString &adapterName)
{
    auto result = sendCommand(HelperCommand::changeIcs, adapterName.toStdWString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::stopIcs()
{
    auto result = sendCommand(HelperCommand::stopIcs);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

QString Helper_win::enableBFE()
{
    auto result = sendCommand(HelperCommand::enableBFE);
    std::wstring log;
    deserializeAnswer(result, log);
    return QString::fromStdWString(log);
}

QString Helper_win::resetAndStartRAS()
{
    auto result = sendCommand(HelperCommand::resetAndStartRAS);
    std::wstring log;
    deserializeAnswer(result, log);
    return QString::fromStdWString(log);
}

void Helper_win::setIPv6EnabledInFirewall(bool b)
{
    sendCommand(HelperCommand::setIPv6EnabledInFirewall, b);
}

void Helper_win::setFirewallOnBoot(bool b)
{
    sendCommand(HelperCommand::setFirewallOnBoot, b);
}

bool Helper_win::addHosts(const QString &hosts)
{
    auto result = sendCommand(HelperCommand::addHosts, hosts.toStdWString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::removeHosts()
{
    auto result = sendCommand(HelperCommand::removeHosts);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

void Helper_win::closeAllTcpConnections(bool isKeepLocalSockets)
{
    sendCommand(HelperCommand::closeAllTcpConnections, isKeepLocalSockets);
}

QStringList Helper_win::getProcessesList()
{
    auto result = sendCommand(HelperCommand::getProcessesList);
    std::vector<std::wstring> list;
    deserializeAnswer(result, list);
    return toStringList(list);
}

bool Helper_win::whitelistPorts(const QString &ports)
{
    auto result = sendCommand(HelperCommand::whitelistPorts, ports.toStdWString());
    bool success;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_win::deleteWhitelistPorts()
{
    auto result = sendCommand(HelperCommand::deleteWhitelistPorts);
    bool success;
    deserializeAnswer(result, success);
    return success;
}

void Helper_win::enableDnsLeaksProtection()
{
    sendCommand(HelperCommand::enableDnsLeaksProtection, toStdVector(customDnsIp_));
}

void Helper_win::disableDnsLeaksProtection()
{
    sendCommand(HelperCommand::disableDnsLeaksProtection);
}

void Helper_win::reinstallWanIkev2()
{
    sendCommand(HelperCommand::reinstallWanIkev2);
}

void Helper_win::enableWanIkev2()
{
    sendCommand(HelperCommand::enableWanIkev2);
}

void Helper_win::setMacAddressRegistryValueSz(const QString &subkeyInterfaceName, const QString &value)
{
    sendCommand(HelperCommand::setMacAddressRegistryValueSz, subkeyInterfaceName.toStdWString(), value.toStdWString());
}

void Helper_win::removeMacAddressRegistryProperty(const QString &subkeyInterfaceName)
{
    sendCommand(HelperCommand::removeMacAddressRegistryProperty, subkeyInterfaceName.toStdWString());
}

void Helper_win::resetNetworkAdapter(const QString &subkeyInterfaceName, bool bringAdapterBackUp)
{
    sendCommand(HelperCommand::resetNetworkAdapter, subkeyInterfaceName.toStdWString(), bringAdapterBackUp);
}

void Helper_win::addIKEv2DefaultRoute()
{
    sendCommand(HelperCommand::addIKEv2DefaultRoute);
}

void Helper_win::removeWindscribeNetworkProfiles()
{
    sendCommand(HelperCommand::removeWindscribeNetworkProfiles);
}

void Helper_win::setIKEv2IPSecParameters()
{
    sendCommand(HelperCommand::setIKEv2IPSecParameters);
}

bool Helper_win::makeHostsFileWritable()
{
    auto result = sendCommand(HelperCommand::makeHostsFileWritable);
    bool success;
    deserializeAnswer(result, success);
    if (success) {
        qCInfo(LOG_BASIC) << "\"hosts\" file is writable now.";
    } else {
        qCWarning(LOG_BASIC) << "Was not able to change \"hosts\" file permissions from read-only.";
    }
    return success;
}

void Helper_win::setCustomDnsIps(const QStringList &ips)
{
    customDnsIp_ = ips;
}

void Helper_win::createOpenVpnAdapter(bool useDCODriver)
{
    sendCommand(HelperCommand::createOpenVpnAdapter, useDCODriver);
}

void Helper_win::removeOpenVpnAdapter()
{
    sendCommand(HelperCommand::removeOpenVpnAdapter);
}

void Helper_win::disableDohSettings()
{
    sendCommand(HelperCommand::disableDohSettings);
}

void Helper_win::enableDohSettings()
{
    sendCommand(HelperCommand::enableDohSettings);
}

unsigned long Helper_win::ssidFromInterfaceGUID(const QString &interfaceGUID, QString &ssid)
{
    auto result = sendCommand(HelperCommand::ssidFromInterfaceGUID, interfaceGUID.toStdWString());
    unsigned long exitCode;
    bool success;
    std::string temp;
    deserializeAnswer(result, success, exitCode, temp);
    if (success) {
        ssid = QString::fromStdString(temp);
        return 0;
    }
    return exitCode;
}

void Helper_win::setNetworkCategory(const QString &networkName, NETWORK_CATEGORY category)
{
    sendCommand(HelperCommand::setNetworkCategory, networkName.toStdWString(), (int)category);
}
