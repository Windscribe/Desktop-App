#include "helper_posix.h"
#include "convert_utils.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "utils/extraconfig.h"
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

void Helper_posix::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isAllowLanTraffic, const QStringList &files, const QStringList &ips, const QStringList &hosts)
{
    sendCommand(HelperCommand::setSplitTunnelingSettings, isActive, isExclude, isAllowLanTraffic,
                toStdVector(files), toStdVector(ips), toStdVector(hosts));
}

bool Helper_posix::sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter, const QString &connectedIp, const types::Protocol &protocol)
{
    auto fillAdapterInfo = [](const AdapterGatewayInfo &a, ADAPTER_GATEWAY_INFO &out) {
        out.adapterName   = a.adapterName().toStdString();
        out.adapterNameV6 = a.adapterNameV6().toStdString();
        out.adapterIp   = a.adapterIpV4();
        out.adapterIpV6 = a.adapterIpV6();
        out.gatewayIp   = a.gatewayV4();
        out.gatewayIpV6 = a.gatewayV6();
        out.ifIndex     = a.ifIndex();
        const auto dns = a.dnsServers();
        for (const auto &ip : dns) {
            out.dnsServers.push_back(ip);
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
                types::IpAddress(connectedIp.toStdString()), vpnAdapter.remoteIp());
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

void Helper_posix::changeMtu(const QString &adapter, int mtu)
{
    sendCommand(HelperCommand::changeMtu, adapter.toStdString(), mtu);
}

bool Helper_posix::executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort)
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
                              socksProxy.toStdString(), socksPort, dnsManager);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::executeTaskKill(CmdKillTarget target)
{
    auto result = sendCommand(HelperCommand::executeTaskKill, target);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::startWireGuard(bool isAmneziaWG)
{
    // At this point in time, the macOS helper treats vanilla WG and AmneziaWG connections the same.
#if defined(Q_OS_LINUX)
    auto result = sendCommand(HelperCommand::startWireGuard, isAmneziaWG, ExtraConfig::instance().getWireGuardVerboseLogging());
#else
    auto result = sendCommand(HelperCommand::startWireGuard, ExtraConfig::instance().getWireGuardVerboseLogging());
#endif
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::stopWireGuard()
{
    auto result = sendCommand(HelperCommand::stopWireGuard);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::configureWireGuard(const WireGuardConfig &config)
{
    if (!config.hasValidAmneziawgParams()) {
        return false;
    }
    std::string clientPrivateKey = QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().data();
    std::string clientIpAddress = config.clientIpAddress().toLatin1().data();
    std::string clientDnsAddressList = config.clientDnsAddress().toLatin1().data();
    std::string peerPublicKey = QByteArray::fromBase64(config.peerPublicKey().toLatin1()).toHex().data();
    std::string peerPresharedKey = QByteArray::fromBase64(config.peerPresharedKey().toLatin1()).toHex().data();
    std::string peerEndpoint = config.peerEndpoint().toLatin1().data();
    std::string allowedIps = config.peerAllowedIps().toLatin1().data();
    uint16_t listenPort = config.clientListenPort().toUInt();

    CmdDnsManager dnsManager = kResolvConf;
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

    AmneziawgConfig amneziawgConfig = config.amneziawgParamToHelperConfig();

    auto result = sendCommand(HelperCommand::configureWireGuard, clientPrivateKey, clientIpAddress, clientDnsAddressList,
                              peerPublicKey, peerPresharedKey, peerEndpoint, allowedIps, listenPort, dnsManager, amneziawgConfig);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::getWireGuardStatus(types::WireGuardStatus *status)
{
    if (status) {
        status->state = types::WireGuardState::NONE;
        status->errorCode = 0;
        status->bytesReceived = status->bytesTransmitted = 0;
    }
    auto result = sendCommand(HelperCommand::getWireGuardStatus);

    unsigned int errorCode = 0;
    WireGuardServiceState state = kWgStateNone;
    unsigned long long bytesReceived = 0, bytesTransmitted = 0;
    deserializeAnswer(result, errorCode, state, bytesReceived, bytesTransmitted);

    switch (state) {
    default:
    case kWgStateNone:
        status->state = types::WireGuardState::NONE;
        break;
    case kWgStateError:
        status->state = types::WireGuardState::FAILURE;
        status->errorCode = errorCode;
        break;
    case kWgStateStarting:
        status->state = types::WireGuardState::STARTING;
        break;
    case kWgStateListening:
        status->state = types::WireGuardState::LISTENING;
        break;
    case kWgStateConnecting:
        status->state = types::WireGuardState::CONNECTING;
        break;
    case kWgStateActive:
        status->state = types::WireGuardState::ACTIVE;
        status->bytesReceived = bytesReceived;
        status->bytesTransmitted = bytesTransmitted;
        break;
    }
    return true;
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

bool Helper_posix::checkFirewallState()
{
    auto result = sendCommand(HelperCommand::checkFirewallState);
    bool isEnabled = false;
    deserializeAnswer(result, isEnabled);
    return isEnabled;
}

bool Helper_posix::clearFirewallRules(bool isKeepPfEnabled)
{
    auto result = sendCommand(HelperCommand::clearFirewallRules, isKeepPfEnabled);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::setFirewallRules(const FirewallConfig &config)
{
    auto result = sendCommand(HelperCommand::setFirewallRules, config);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::getFirewallRules(CmdFirewallRulesQuery query, QString &rules)
{
    auto result = sendCommand(HelperCommand::getFirewallRules, query);
    std::string tempRules;
    deserializeAnswer(result, tempRules);
    rules = QString::fromStdString(tempRules);
    return true;
}

bool Helper_posix::setFirewallOnBoot(bool enabled, const QSet<QString> &ipTable, bool allowLanTraffic)
{
    std::vector<std::string> ips;
    ips.reserve(ipTable.size());
    for (const auto& ip : ipTable) {
        ips.push_back(ip.toStdString());
    }
    sendCommand(HelperCommand::setFirewallOnBoot, enabled, allowLanTraffic, ips);
    return true;
}

bool Helper_posix::startStunnel(const QString &hostname, unsigned int port, unsigned int localPort, bool extraPadding, const QString &customSni)
{
    auto result = sendCommand(HelperCommand::startStunnel, hostname.toStdString(), port, localPort, extraPadding, customSni.toStdString());
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_posix::startWstunnel(const QString &hostname, unsigned int port, unsigned int localPort, const QString &customSni)
{
    auto result = sendCommand(HelperCommand::startWstunnel, hostname.toStdString(), port, localPort, customSni.toStdString());
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

bool Helper_posix::clearWifiHistoryData()
{
    auto result = sendCommand(HelperCommand::clearWifiHistoryData);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

void Helper_posix::installerCleanupStaged()
{
    sendCommand(HelperCommand::installerCleanupStaged);
}

