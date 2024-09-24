#include "helper_posix.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include "types/wireguardtypes.h"
#include "../openvpnversioncontroller.h"
#include "installhelper_mac.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "types/wireguardtypes.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "utils/ws_assert.h"
#include "utils/macutils.h"
#include "utils/executable_signature/executable_signature.h"
#include "../../../../backend/posix_common/helper_commands_serialize.h"

#ifdef Q_OS_LINUX
    #include "utils/dnsscripts_linux.h"
#endif

#define SOCK_PATH "/var/run/windscribe/helper.sock"

using namespace boost::asio;

Helper_posix *g_this_ = NULL;

Helper_posix::Helper_posix(QObject *parent) : IHelper(parent), bIPV6State_(true), cmdId_(0), lastOpenVPNCmdId_(0)
  , ep_(SOCK_PATH), bHelperConnectedEmitted_(false)
  , curState_(STATE_INIT), bNeedFinish_(false), firstConnectToHelperErrorReported_(false)
{
    WS_ASSERT(g_this_ == NULL);
    g_this_ = this;
}

Helper_posix::~Helper_posix()
{
    io_service_.stop();
    setNeedFinish();
    wait();
    g_this_ = NULL;
}

Helper_posix::STATE Helper_posix::currentState() const
{
    return curState_;
}

void Helper_posix::setNeedFinish()
{
    bNeedFinish_ = true;
}

void Helper_posix::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    QMutexLocker locker(&mutex_);

    outFinished = false;
    if (curState_ != STATE_CONNECTED)
    {
        return;
    }

    CMD_GET_CMD_STATUS cmd;
    cmd.cmdId = cmdId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_GET_CMD_STATUS, stream.str(), answer) || answer.executed == 0) {
        doDisconnectAndReconnect();
        return;
    }

    outFinished = (answer.executed == 1);
    outLog = QString::fromStdString(answer.body);
}

void Helper_posix::clearUnblockingCmd(unsigned long cmdId)
{
    Q_UNUSED(cmdId);

    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return;
    }

    CMD_CLEAR_CMDS cmd;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_CLEAR_CMDS, stream.str(), answer)) {
        doDisconnectAndReconnect();
    }
}

void Helper_posix::suspendUnblockingCmd(unsigned long cmdId)
{
    // On Mac, this is the same as clearing a cmd.
    clearUnblockingCmd(cmdId);
}

bool Helper_posix::setSplitTunnelingSettings(bool isActive, bool isExclude,
                                           bool isAllowLanTraffic, const QStringList &files,
                                           const QStringList &ips, const QStringList &hosts)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return false;
    }

    CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
    cmdSplitTunnelingSettings.isActive = isActive;
    cmdSplitTunnelingSettings.isExclude = isExclude;
    cmdSplitTunnelingSettings.isAllowLanTraffic = isAllowLanTraffic;

    for (int i = 0; i < files.count(); ++i) {
        cmdSplitTunnelingSettings.files.push_back(files[i].toStdString());
    }

    for (int i = 0; i < ips.count(); ++i) {
        cmdSplitTunnelingSettings.ips.push_back(ips[i].toStdString());
    }

    for (int i = 0; i < hosts.count(); ++i) {
        cmdSplitTunnelingSettings.hosts.push_back(hosts[i].toStdString());
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSplitTunnelingSettings;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_SPLIT_TUNNELING_SETTINGS, stream.str(), answer)) {
        doDisconnectAndReconnect();
        return false;
    }
    return true;
}

bool Helper_posix::sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                                     const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                     const QString &connectedIp, const types::Protocol &protocol)
{
    Q_UNUSED(isTerminateSocket);
    Q_UNUSED(isKeepLocalSocket);
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return false;
    }

    CMD_SEND_CONNECT_STATUS cmd;
    cmd.isConnected = isConnected;

    if (isConnected) {
        if (protocol.isStunnelOrWStunnelProtocol()) {
            cmd.protocol = kCmdProtocolStunnelOrWstunnel;
        }
        else if (protocol.isIkev2Protocol()) {
            cmd.protocol = kCmdProtocolIkev2;
        }
        else if (protocol.isWireGuardProtocol()) {
            cmd.protocol = kCmdProtocolWireGuard;
        }
        else if (protocol.isOpenVpnProtocol()) {
            cmd.protocol = kCmdProtocolOpenvpn;
        }
        else {
            WS_ASSERT(false);
        }

        auto fillAdapterInfo = [](const AdapterGatewayInfo &a, ADAPTER_GATEWAY_INFO &out) {
            out.adapterName = a.adapterName().toStdString();
            out.adapterIp = a.adapterIp().toStdString();
            out.gatewayIp = a.gateway().toStdString();
            const QStringList dns = a.dnsServers();
            for(auto ip : dns) {
                out.dnsServers.push_back(ip.toStdString());
            }
        };

        fillAdapterInfo(defaultAdapter, cmd.defaultAdapter);
        fillAdapterInfo(vpnAdapter, cmd.vpnAdapter);

        cmd.connectedIp = connectedIp.toStdString();
        cmd.remoteIp = vpnAdapter.remoteIp().toStdString();
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_SEND_CONNECT_STATUS, stream.str(), answer)) {
        doDisconnectAndReconnect();
        return false;
    }
    if (answer.executed == 0) {
        return false;
    }

    return true;
}

bool Helper_posix::changeMtu(const QString &adapter, int mtu)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_CHANGE_MTU cmd;
    cmd.mtu = mtu;
    cmd.adapterName = adapter.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_CHANGE_MTU, stream.str(), answer);
}

bool Helper_posix::deleteRoute(const QString &range, int mask, const QString &gateway)
{
    QMutexLocker locker(&mutex_);

    CMD_ANSWER answer;
    CMD_DELETE_ROUTE cmd;
    cmd.range = range.toStdString();
    cmd.mask = mask;
    cmd.gateway = gateway.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_DELETE_ROUTE, stream.str(), answer);
}

IHelper::ExecuteError Helper_posix::startWireGuard()
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return IHelper::EXECUTE_ERROR;
    }

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_START_WIREGUARD, std::string(), answer) || answer.executed == 0) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }

    return IHelper::EXECUTE_SUCCESS;
}

bool Helper_posix::stopWireGuard()
{
    if (curState_ == STATE_CONNECTED) {
        QMutexLocker locker(&mutex_);

        CMD_ANSWER answer;
        if (!runCommand(HELPER_CMD_STOP_WIREGUARD, "", answer)) {
            doDisconnectAndReconnect();
            return false;
        }
        if (answer.executed == 0) {
            return false;
        }
    }
    return true;
}

bool Helper_posix::configureWireGuard(const WireGuardConfig &config)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED)
        return false;

    CMD_CONFIGURE_WIREGUARD cmd;
    cmd.clientPrivateKey =
        QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().data();
    cmd.clientIpAddress = config.clientIpAddress().toLatin1().data();
    cmd.clientDnsAddressList = config.clientDnsAddress().toLatin1().data();
#if defined(Q_OS_LINUX)
    switch (DnsScripts_linux::instance().dnsManager()) {
    case DnsScripts_linux::SCRIPT_TYPE::SYSTEMD_RESOLVED:
        cmd.dnsManager = kSystemdResolved;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::RESOLV_CONF:
        cmd.dnsManager = kResolvConf;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::NETWORK_MANAGER:
    default:
        cmd.dnsManager = kNetworkManager;
        break;
    }
#endif

    cmd.peerEndpoint = config.peerEndpoint().toLatin1().data();
    cmd.peerPublicKey = QByteArray::fromBase64(config.peerPublicKey().toLatin1()).toHex().data();
    cmd.peerPresharedKey
        = QByteArray::fromBase64(config.peerPresharedKey().toLatin1()).toHex().data();
    cmd.allowedIps = config.peerAllowedIps().toLatin1().data();
    cmd.listenPort = config.clientListenPort().toUInt();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_CONFIGURE_WIREGUARD, stream.str(), answer) || answer.executed == 0) {
        qCDebug(LOG_WIREGUARD) << "WireGuard configuration failed";
        doDisconnectAndReconnect();
        return false;
    }
    return true;
}

bool Helper_posix::getWireGuardStatus(types::WireGuardStatus *status)
{
    QMutexLocker locker(&mutex_);

    if (status) {
        status->state = types::WireGuardState::NONE;
        status->errorCode = 0;
        status->bytesReceived = status->bytesTransmitted = 0;
    }
    if (curState_ != STATE_CONNECTED) {
        return false;
    }

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_GET_WIREGUARD_STATUS, "", answer)) {
        doDisconnectAndReconnect();
        return false;
    }
    if (!answer.executed) {
        return false;
    }

    switch (answer.cmdId) {
    default:
    case kWgStateNone:
        status->state = types::WireGuardState::NONE;
        break;
    case kWgStateError:
        status->state = types::WireGuardState::FAILURE;
        status->errorCode = answer.customInfoValue[0];
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
        status->bytesReceived = answer.customInfoValue[0];
        status->bytesTransmitted = answer.customInfoValue[1];
        break;
    }
    return true;
}

bool Helper_posix::startCtrld(const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog)
{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return false;
    }

    std::vector<std::string> domainsList;

    CMD_START_CTRLD cmd;
    cmd.upstream1 = upstream1.toStdString();
    cmd.upstream2 = upstream2.toStdString();
    for (auto domain : domains) {
        domainsList.push_back(domain.toStdString());
    }
    cmd.domains = domainsList;
    cmd.isCreateLog = isCreateLog;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_START_CTRLD, stream.str(), answer) || answer.executed == 0) {
        qCDebug(LOG_BASIC) << "helper returned error starting ctrld";
        doDisconnectAndReconnect();
        return false;
    }

    return true;
}

bool Helper_posix::stopCtrld()
{
    return executeTaskKill(kTargetCtrld);
}

IHelper::ExecuteError Helper_posix::executeOpenVPN(const QString &config, unsigned int port, const QString &httpProxy, unsigned int httpPort,
                                                   const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId, bool isCustomConfig)

{
    QMutexLocker locker(&mutex_);

    if (curState_ != STATE_CONNECTED) {
        return IHelper::EXECUTE_ERROR;
    }

    CMD_START_OPENVPN cmd;
    cmd.config = config.toStdString();
    cmd.port = port;
    cmd.httpProxy = httpProxy.toStdString();
    cmd.socksProxy = socksProxy.toStdString();
    cmd.httpPort = httpPort;
    cmd.socksPort = socksPort;
    cmd.isCustomConfig = isCustomConfig;

#if defined(Q_OS_LINUX)
    switch (DnsScripts_linux::instance().dnsManager()) {
    case DnsScripts_linux::SCRIPT_TYPE::SYSTEMD_RESOLVED:
        cmd.dnsManager = kSystemdResolved;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::RESOLV_CONF:
        cmd.dnsManager = kResolvConf;
        break;
    case DnsScripts_linux::SCRIPT_TYPE::NETWORK_MANAGER:
    default:
        cmd.dnsManager = kNetworkManager;
        break;
    }
#endif

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_START_OPENVPN, stream.str(), answer) || answer.executed == 0) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }

    outCmdId = answer.cmdId;
    return IHelper::EXECUTE_SUCCESS;
}

bool Helper_posix::executeTaskKill(CmdKillTarget target)
{
    QMutexLocker locker(&mutex_);

    CMD_TASK_KILL cmd;
    CMD_ANSWER answer;
    cmd.target = target;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_TASK_KILL, stream.str(), answer);
}

bool Helper_posix::setDnsScriptEnabled(bool bEnabled)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_DNS_SCRIPT_ENABLED cmd;
    CMD_ANSWER answer;
    cmd.enabled = bEnabled;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_DNS_SCRIPT_ENABLED, stream.str(), answer);
}

bool Helper_posix::checkFirewallState(const QString &tag)
{
    QMutexLocker locker(&mutex_);

    CMD_CHECK_FIREWALL_STATE cmd;
    CMD_ANSWER answer;
    cmd.tag = tag.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!runCommand(HELPER_CMD_CHECK_FIREWALL_STATE, stream.str(), answer)) {
        return false;
    }
    return answer.exitCode != 0;
}

bool Helper_posix::clearFirewallRules(bool isKeepPfEnabled)
{
    QMutexLocker locker(&mutex_);

    CMD_CLEAR_FIREWALL_RULES cmd;
    CMD_ANSWER answer;
    cmd.isKeepPfEnabled = isKeepPfEnabled;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_CLEAR_FIREWALL_RULES, stream.str(), answer);
}

bool Helper_posix::setFirewallRules(CmdIpVersion version, const QString &table, const QString &group, const QString &rules)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_FIREWALL_RULES cmd;
    CMD_ANSWER answer;
    cmd.ipVersion = version;
    cmd.table = table.toStdString();
    cmd.group = group.toStdString();
    cmd.rules = rules.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_FIREWALL_RULES, stream.str(), answer);
}

bool Helper_posix::getFirewallRules(CmdIpVersion version, const QString &table, const QString &group, QString &rules)
{
    QMutexLocker locker(&mutex_);

    CMD_GET_FIREWALL_RULES cmd;
    CMD_ANSWER answer;
    cmd.ipVersion = version;
    cmd.table = table.toStdString();
    cmd.group = group.toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    if (!runCommand(HELPER_CMD_GET_FIREWALL_RULES, stream.str(), answer)) {
        return false;
    }
    rules = QString::fromStdString(answer.body);
    return true;
}

bool Helper_posix::setFirewallOnBoot(bool enabled, const QSet<QString> &ipTable, bool allowLanTraffic)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_FIREWALL_ON_BOOT cmd;
    CMD_ANSWER answer;
    cmd.enabled = enabled;
    cmd.allowLanTraffic = allowLanTraffic;

    std::string ipTableStr = "";
    for (const auto& ip : ipTable) {
        ipTableStr += ip.toStdString();
        ipTableStr += " ";
    }

    cmd.ipTable = ipTableStr;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_FIREWALL_ON_BOOT, stream.str(), answer);
}

bool Helper_posix::setMacAddress(const QString &interface, const QString &macAddress, const QString &network, bool isWifi)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_ADDRESS cmd;
    CMD_ANSWER answer;
    cmd.interface = interface.toStdString();
    cmd.macAddress = macAddress.toStdString();
    cmd.network = network.toStdString();
    cmd.isWifi = isWifi;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    return runCommand(HELPER_CMD_SET_MAC_ADDRESS, stream.str(), answer) && answer.executed;
}

bool Helper_posix::startStunnel(const QString &hostname, unsigned int port, unsigned int localPort, bool extraPadding)
{
    QMutexLocker locker(&mutex_);

    CMD_START_STUNNEL cmd;
    cmd.hostname = hostname.toStdString();
    cmd.port = port;
    cmd.localPort = localPort;
    cmd.extraPadding = extraPadding;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_START_STUNNEL, stream.str(), answer) || answer.executed == 0) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }

    return IHelper::EXECUTE_SUCCESS;
}

bool Helper_posix::startWstunnel(const QString &hostname, unsigned int port, unsigned int localPort)
{
    QMutexLocker locker(&mutex_);

    CMD_START_WSTUNNEL cmd;
    cmd.hostname = hostname.toStdString();
    cmd.port = port;
    cmd.localPort = localPort;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    CMD_ANSWER answer;
    if (!runCommand(HELPER_CMD_START_WSTUNNEL, stream.str(), answer) || answer.executed == 0) {
        doDisconnectAndReconnect();
        return IHelper::EXECUTE_ERROR;
    }

    return IHelper::EXECUTE_SUCCESS;
}

void Helper_posix::run()
{
    BIND_CRASH_HANDLER_FOR_THREAD();
    firstConnectToHelperErrorReported_ = false;
    io_service_.reset();
    reconnectElapsedTimer_.start();
    g_this_->socket_.reset(new boost::asio::local::stream_protocol::socket(io_service_));
    socket_->async_connect(ep_, connectHandler);
    io_service_.run();
}

void Helper_posix::connectHandler(const boost::system::error_code &ec)
{
    if (!ec) {
        // we connected
        g_this_->curState_ = STATE_CONNECTED;
        //emit signal only once on first run
        if (!g_this_->bHelperConnectedEmitted_) {
            g_this_->bHelperConnectedEmitted_ = true;
        }
        qCDebug(LOG_BASIC) << "connected to helper socket";
    } else {
        // only report the first error while connecting to helper in order to prevent log bloat
        // we want to see first error in case it is different than all following errors
        if (!g_this_->firstConnectToHelperErrorReported_) {
            qCDebug(LOG_BASIC) << "Error while connecting to helper (first): " << ec.value();
            g_this_->firstConnectToHelperErrorReported_ = true;
        }

        if (g_this_->reconnectElapsedTimer_.elapsed() > MAX_WAIT_HELPER) {
            if (!g_this_->bHelperConnectedEmitted_) {
                qCDebug(LOG_BASIC) << "Error while connecting to helper: " << ec.value();
                g_this_->curState_ = STATE_FAILED_CONNECT;
            } else {
                emit g_this_->lostConnectionToHelper();
            }
        } else {
            // try reconnect
            msleep(10);
            {
                QMutexLocker locker(&g_this_->mutexSocket_);
                g_this_->socket_->async_connect(g_this_->ep_, connectHandler);
            }
        }
    }
}

void Helper_posix::doDisconnectAndReconnect()
{
    if (!isRunning())
    {
        qCDebug(LOG_BASIC) << "Disconnected from helper socket, try reconnect";
        g_this_->curState_ = STATE_INIT;
        start(QThread::LowPriority);
    }
}

bool Helper_posix::runCommand(int cmdId, const std::string &data, CMD_ANSWER &answer)
{
    bool ret = sendCmdToHelper(cmdId, data);
    if (!ret) {
        return ret;
    }

    return readAnswer(answer);
}

bool Helper_posix::readAnswer(CMD_ANSWER &outAnswer)
{
    boost::system::error_code ec;
    int length;
    boost::asio::read(*socket_, boost::asio::buffer(&length, sizeof(length)),
                      boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec) {
        return false;
    } else {
        std::vector<char> buff(length);
        boost::asio::read(*socket_, boost::asio::buffer(&buff[0], length),
                          boost::asio::transfer_exactly(length), ec);
        if (ec) {
            return false;
        }

        std::string str(buff.begin(), buff.end());
        std::istringstream stream(str);
        boost::archive::text_iarchive ia(stream, boost::archive::no_header);
        ia >> outAnswer;
    }

    return true;
}

bool Helper_posix::sendCmdToHelper(int cmdId, const std::string &data)
{
    int length = data.size();
    boost::system::error_code ec;

    // first 4 bytes - cmdId
    boost::asio::write(*socket_, boost::asio::buffer(&cmdId, sizeof(cmdId)), boost::asio::transfer_exactly(sizeof(cmdId)), ec);
    if (ec) {
        return false;
    }
    // second 4 bytes - pid
    const auto pid = getpid();
    boost::asio::write(*socket_, boost::asio::buffer(&pid, sizeof(pid)), boost::asio::transfer_exactly(sizeof(pid)), ec);
    if (ec) {
        return false;
    }
    // third 4 bytes - size of buffer
    boost::asio::write(*socket_, boost::asio::buffer(&length, sizeof(length)), boost::asio::transfer_exactly(sizeof(length)), ec);
    if (ec) {
        return false;
    }
    // body of message
    boost::asio::write(*socket_, boost::asio::buffer(data.data(), length), boost::asio::transfer_exactly(length), ec);
    if (ec) {
        doDisconnectAndReconnect();
        return false;
    }

    return true;
}
