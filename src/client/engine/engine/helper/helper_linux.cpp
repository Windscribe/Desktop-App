#include "helper_linux.h"
#include <QProcess>
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "utils/dnsscripts_linux.h"
#include "utils/log/categories.h"

Helper_linux::Helper_linux(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger) : Helper_posix(std::move(backend), logger)
{
}

bool Helper_linux::installUpdate(const QString &package) const
{
    QProcess process;
    process.setProgram("/opt/windscribe/scripts/install-update");
    process.setArguments(QStringList() << package);
    int ret = process.startDetached();
    if (!ret) {
        qCCritical(LOG_AUTO_UPDATER) << "Install failed to start" << ret;
        return false;
    }
    return true;
}

void Helper_linux::setDnsLeakProtectEnabled(bool bEnabled)
{
    sendCommand(HelperCommand::setDnsLeakProtectEnabled, bEnabled);
}

void Helper_linux::resetMacAddresses(const QString &ignoreNetwork)
{
    sendCommand(HelperCommand::resetMacAddresses, ignoreNetwork.toStdString());
}

bool Helper_linux::startWireGuard()
{
    auto result = sendCommand(HelperCommand::startWireGuard);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_linux::stopWireGuard()
{
    auto result = sendCommand(HelperCommand::stopWireGuard);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_linux::configureWireGuard(const WireGuardConfig &config)
{
    std::string clientPrivateKey = QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().data();
    std::string clientIpAddress = config.clientIpAddress().toLatin1().data();
    std::string clientDnsAddressList = config.clientDnsAddress().toLatin1().data();
    std::string peerPublicKey = QByteArray::fromBase64(config.peerPublicKey().toLatin1()).toHex().data();
    std::string peerPresharedKey = QByteArray::fromBase64(config.peerPresharedKey().toLatin1()).toHex().data();
    std::string peerEndpoint = config.peerEndpoint().toLatin1().data();
    std::string allowedIps = config.peerAllowedIps().toLatin1().data();
    uint16_t listenPort = config.clientListenPort().toUInt();

    CmdDnsManager dnsManager = kResolvConf;
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

    auto result = sendCommand(HelperCommand::configureWireGuard, clientPrivateKey, clientIpAddress, clientDnsAddressList,
                              peerPublicKey, peerPresharedKey, peerEndpoint, allowedIps, listenPort, dnsManager);
    bool success = false;
    deserializeAnswer(result, success);
    return success;
}

bool Helper_linux::getWireGuardStatus(types::WireGuardStatus *status)
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
        status->bytesTransmitted = bytesReceived;
        break;
    }
    return true;
}