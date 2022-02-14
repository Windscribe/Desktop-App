#include "utils/boost_includes.h"
#include "utils/executable_signature/executable_signature.h"
#include "helper_win.h"
#include "utils/crashhandler.h"
#include "utils/logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <sstream>
#include "windscribeinstallhelper_win.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/types/wireguardconfig.h"
#include "engine/types/wireguardtypes.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/types/protocoltype.h"
#include "utils/win32handle.h"

#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

//  the program to connect to the helper socket without starting the service (uncomment for debug purpose)
// #define DEBUG_DONT_USE_SERVICE

SC_HANDLE schSCManager_ = NULL;
SC_HANDLE schService_ = NULL;

Helper_win::Helper_win(QObject *parent) : IHelper(parent)
{
    initVariables();
}

Helper_win::~Helper_win()
{
    bStopThread_ = true;
    wait();

#if defined QT_DEBUG
    if (curState_ == STATE_CONNECTED)
    {
        debugGetActiveUnblockingCmdCount();
        qCDebug(LOG_BASIC) << "Active unblocking commands in helper on exit:" << debugGetActiveUnblockingCmdCount();
    }
#endif

    if (schService_)
    {
        CloseServiceHandle(schService_);
    }
    if (schSCManager_)
    {
        CloseServiceHandle(schSCManager_);
    }
}

void Helper_win::startInstallHelper()
{
    initVariables();
    Q_ASSERT(schSCManager_ == NULL);
    Q_ASSERT(schService_ == NULL);

    helperLabel_ = "WindscribeService";

    // check WindscribeService.exe signature
    QString serviceExePath = QCoreApplication::applicationDirPath() + "/WindscribeService.exe";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(serviceExePath.toStdWString()))
    {
        qCDebug(LOG_BASIC) << "WindscribeService signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        curState_ = STATE_USER_CANCELED;
        return;
    }

    start(QThread::LowPriority);
}

Helper_win::STATE Helper_win::currentState() const
{
    return curState_;
}

bool Helper_win::reinstallHelper()
{
    QString servicePath = QCoreApplication::applicationDirPath() + "/WindscribeService.exe";
    QString subinaclPath = QCoreApplication::applicationDirPath() + "/subinacl.exe";
    return WindscribeInstallHelper_win::executeInstallHelperCmd(servicePath, subinaclPath);
}

void Helper_win::setNeedFinish()
{
    //nothing todo for Windows
}

QString Helper_win::getHelperVersion()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_GET_HELPER_VERSION, std::string());
    if (mpr.success)
    {
        return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
    }
    else
    {
        return "failed detect";
    }
}

void Helper_win::getUnblockingCmdStatus(unsigned long cmdId, QString &outLog, bool &outFinished)
{
    QMutexLocker locker(&mutex_);

    CMD_CHECK_UNBLOCKING_CMD_STATUS cmdCheckUnblockingCmdStatus;
    cmdCheckUnblockingCmdStatus.cmdId = cmdId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdCheckUnblockingCmdStatus;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CHECK_UNBLOCKING_CMD_STATUS, stream.str());

    if (mpr.success)
    {
        outFinished = mpr.blockingCmdFinished;
        if (outFinished)
        {
            outLog = QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
        }
    }
    else
    {
        outFinished = false;
        outLog.clear();
    }
}

void Helper_win::clearUnblockingCmd(unsigned long cmdId)
{
    QMutexLocker locker(&mutex_);

    CMD_CLEAR_UNBLOCKING_CMD cmdClearUnblockingCmd;
    cmdClearUnblockingCmd.blockingCmdId = cmdId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdClearUnblockingCmd;

    sendCmdToHelper(AA_COMMAND_CLEAR_UNBLOCKING_CMD, stream.str());
}

void Helper_win::suspendUnblockingCmd(unsigned long cmdId)
{
    QMutexLocker locker(&mutex_);

    CMD_SUSPEND_UNBLOCKING_CMD cmdSuspendUnblockingCmd;
    cmdSuspendUnblockingCmd.blockingCmdId = cmdId;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSuspendUnblockingCmd;

    sendCmdToHelper(AA_COMMAND_SUSPEND_UNBLOCKING_CMD, stream.str());
}

bool Helper_win::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isKeepLocalSockets,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts)
{
    QMutexLocker locker(&mutex_);

    CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
    cmdSplitTunnelingSettings.isActive = isActive;
    cmdSplitTunnelingSettings.isExclude = isExclude;
    cmdSplitTunnelingSettings.isKeepLocalSockets = isKeepLocalSockets;

    for (int i = 0; i < files.count(); ++i)
    {
        cmdSplitTunnelingSettings.files.push_back(files[i].toStdWString());
    }

    for (int i = 0; i < ips.count(); ++i)
    {
        cmdSplitTunnelingSettings.ips.push_back(ips[i].toStdWString());
    }

    for (int i = 0; i < hosts.count(); ++i)
    {
        cmdSplitTunnelingSettings.hosts.push_back(hosts[i].toStdString());
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSplitTunnelingSettings;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_SPLIT_TUNNELING_SETTINGS, stream.str());
    return mpr.exitCode;
}

void Helper_win::sendConnectStatus(bool isConnected, bool isCloseTcpSocket, bool isKeepLocalSocket, const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const ProtocolType &protocol)
{
    QMutexLocker locker(&mutex_);

    CMD_CONNECT_STATUS cmd;
    cmd.isConnected = isConnected;
    cmd.isCloseTcpSocket = isCloseTcpSocket;
    cmd.isKeepLocalSocket = isKeepLocalSocket;

    if (isConnected)
    {
        if (protocol.isStunnelOrWStunnelProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL;
        }
        else if (protocol.isIkev2Protocol())
        {
            cmd.protocol = CMD_PROTOCOL_IKEV2;
        }
        else if (protocol.isWireGuardProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_WIREGUARD;
        }
        else if (protocol.isOpenVpnProtocol())
        {
            cmd.protocol = CMD_PROTOCOL_OPENVPN;
        }
        else
        {
            Q_ASSERT(false);
        }

        auto fillAdapterInfo = [](const AdapterGatewayInfo &a, ADAPTER_GATEWAY_INFO &out)
        {
            out.adapterName = a.adapterName().toStdString();
            out.adapterIp = a.adapterIp().toStdString();
            out.gatewayIp = a.gateway().toStdString();
            out.ifIndex = a.ifIndex();
            const QStringList dns = a.dnsServers();
            for(auto ip : dns)
            {
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

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CONNECT_STATUS, stream.str());
}

bool Helper_win::setCustomDnsWhileConnected(bool isIkev2, unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    Q_UNUSED(isIkev2)

    QMutexLocker locker(&mutex_);

    CMD_DNS_WHILE_CONNECTED cmd;
    cmd.ifIndex = ifIndex;
    cmd.szDnsIpAddress = overrideDnsIpAddress.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DNS_WHILE_CONNECTED, stream.str());
    return mpr.exitCode == 0;
}

IHelper::ExecuteError Helper_win::startWireGuard(const QString &exeName, const QString &deviceName)
{
    QMutexLocker locker(&mutex_);

    // check executable signature
    QString wireGuardExePath = QCoreApplication::applicationDirPath() + "/" + exeName + ".exe";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(wireGuardExePath.toStdWString()))
    {
        qCDebug(LOG_CONNECTION) << "WireGuard executable signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        return IHelper::EXECUTE_VERIFY_ERROR;
    }

    CMD_START_WIREGUARD cmdStartWireGuard;
    cmdStartWireGuard.szExecutable = exeName.toStdWString();
    cmdStartWireGuard.szDeviceName = deviceName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdStartWireGuard;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_START_WIREGUARD, stream.str());
    return mpr.success ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_win::stopWireGuard()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_STOP_WIREGUARD, std::string());
    if (mpr.success) {
        // Daemon was running, check if it's been terminated.
        if (mpr.blockingCmdFinished && (!mpr.additionalString.empty())) {
            qCDebugMultiline(LOG_WIREGUARD) << "WireGuard daemon output:"
                << QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
        }
        return mpr.blockingCmdFinished;
    }
    // Daemon was not running.
    return true;
}

bool Helper_win::configureWireGuard(const WireGuardConfig &config)
{
    QMutexLocker locker(&mutex_);

    CMD_CONFIGURE_WIREGUARD cmdConfigureWireGuard;
    cmdConfigureWireGuard.clientPrivateKey =
        QByteArray::fromBase64(config.clientPrivateKey().toLatin1()).toHex().toStdString();
    cmdConfigureWireGuard.clientIpAddress = config.clientIpAddress().toStdString();
    cmdConfigureWireGuard.clientDnsAddressList = config.clientDnsAddress().toStdString();
    cmdConfigureWireGuard.peerEndpoint = config.peerEndpoint().toStdString();
    cmdConfigureWireGuard.peerPublicKey =
        QByteArray::fromBase64(config.peerPublicKey().toLatin1()).toHex().toStdString();
    cmdConfigureWireGuard.peerPresharedKey =
        QByteArray::fromBase64(config.peerPresharedKey().toLatin1()).toHex().toStdString();
    cmdConfigureWireGuard.allowedIps = config.peerAllowedIps().toStdString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdConfigureWireGuard;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CONFIGURE_WIREGUARD, stream.str());
    if (!mpr.success) {
        qCDebug(LOG_WIREGUARD) << "WireGuard configuration failed, error code =" << mpr.exitCode;
    }
    return mpr.success;
}

bool Helper_win::getWireGuardStatus(WireGuardStatus *status)
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_GET_WIREGUARD_STATUS, std::string());
    if (mpr.success && status) {
        status->errorCode = 0;
        status->bytesReceived = status->bytesTransmitted = 0;
        switch (mpr.exitCode) {
        default:
        case WIREGUARD_STATE_NONE:
            status->state = WireGuardState::NONE;
            break;
        case WIREGUARD_STATE_ERROR:
            status->state = WireGuardState::FAILURE;
            status->errorCode = mpr.customInfoValue[0];
            break;
        case WIREGUARD_STATE_STARTING:
            status->state = WireGuardState::STARTING;
            break;
        case WIREGUARD_STATE_LISTENING:
            status->state = WireGuardState::LISTENING;
            break;
        case WIREGUARD_STATE_CONNECTING:
            status->state = WireGuardState::CONNECTING;
            break;
        case WIREGUARD_STATE_ACTIVE:
            status->state = WireGuardState::ACTIVE;
            status->bytesReceived = mpr.customInfoValue[0];
            status->bytesTransmitted = mpr.customInfoValue[1];
            break;
        }
    }
    if (!mpr.additionalString.empty()) {
        qCDebugMultiline(LOG_WIREGUARD) << "WireGuard daemon output:"
            << QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
    }
    return mpr.success;
}

void Helper_win::setDefaultWireGuardDeviceName(const QString & /*deviceName*/)
{
    // Nothing to do.
}

bool Helper_win::isHelperConnected() const
{
    return curState_ == STATE_CONNECTED;
}

IHelper::ExecuteError Helper_win::executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId)
{
    QMutexLocker locker(&mutex_);

    // check openvpn executable signature
    QString openVpnExePath = QCoreApplication::applicationDirPath() + "/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(openVpnExePath.toStdWString()))
    {
        qCDebug(LOG_CONNECTION) << "OpenVPN executable signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        return IHelper::EXECUTE_VERIFY_ERROR;
    }

    CMD_RUN_OPENVPN cmdRunOpenVpn;
    cmdRunOpenVpn.szOpenVpnExecutable = OpenVpnVersionController::instance().getSelectedOpenVpnExecutable().toStdWString();
    cmdRunOpenVpn.szConfigPath = configPath.toStdWString();
    cmdRunOpenVpn.portNumber = portNumber;
    cmdRunOpenVpn.szHttpProxy = httpProxy.toStdWString();
    cmdRunOpenVpn.szSocksProxy = socksProxy.toStdWString();
    cmdRunOpenVpn.httpPortNumber = httpPort;
    cmdRunOpenVpn.socksPortNumber = socksPort;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdRunOpenVpn;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RUN_OPENVPN, stream.str());
    outCmdId = mpr.blockingCmdId;

    return mpr.success ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_win::executeTaskKill(const QString &executableName)
{
    QMutexLocker locker(&mutex_);

    CMD_TASK_KILL cmdTaskKill;
    cmdTaskKill.szExecutableName = executableName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdTaskKill;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_TASK_KILL, stream.str());

    return mpr.success;
}

bool Helper_win::executeResetTap(const QString &tapName)
{
    QMutexLocker locker(&mutex_);
    CMD_RESET_TAP cmdResetTap;
    cmdResetTap.szTapName = tapName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdResetTap;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RESET_TAP, stream.str());

    return mpr.success;
}

QString Helper_win::executeSetMetric(const QString &interfaceType, const QString &interfaceName, const QString &metricNumber)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_METRIC cmdSetMetric;
    cmdSetMetric.szInterfaceType = interfaceType.toStdWString();
    cmdSetMetric.szInterfaceName = interfaceName.toStdWString();
    cmdSetMetric.szMetricNumber = metricNumber.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSetMetric;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_SET_METRIC, stream.str());
    return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
}

QString Helper_win::executeWmicEnable(const QString &adapterName)
{
    QMutexLocker locker(&mutex_);

    CMD_WMIC_ENABLE cmdWmicEnable;
    cmdWmicEnable.szAdapterName = adapterName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdWmicEnable;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_WMIC_ENABLE, stream.str());
    return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
}

QString Helper_win::executeWmicGetConfigManagerErrorCode(const QString &adapterName)
{
    QMutexLocker locker(&mutex_);

    CMD_WMIC_GET_CONFIG_ERROR_CODE cmdWmicGetConfigErrorCode;
    cmdWmicGetConfigErrorCode.szAdapterName = adapterName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdWmicGetConfigErrorCode;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_WMIC_GET_CONFIG_ERROR_CODE, stream.str());
    return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
}

bool Helper_win::executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid, unsigned long &outCmdId, const QString &eventName)
{
    QMutexLocker locker(&mutex_);

    // check executable signature
    QString updateIcsExePath = QCoreApplication::applicationDirPath() + "/ChangeIcs.exe";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(updateIcsExePath.toStdWString()))
    {
        qCDebug(LOG_CONNECTION) << "ChangeIcs executable signature incorrect: " << QString::fromStdString(sigCheck.lastError());
        return false;
    }

    CMD_UPDATE_ICS cmdUpdateIcs;
    cmdUpdateIcs.cmd = cmd;
    cmdUpdateIcs.szConfigPath = configPath.toStdWString();
    cmdUpdateIcs.szPublicGuid = publicGuid.toStdWString();
    cmdUpdateIcs.szPrivateGuid = privateGuid.toStdWString();
    cmdUpdateIcs.szEventName = eventName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdUpdateIcs;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_UPDATE_ICS, stream.str());
    outCmdId = mpr.blockingCmdId;

    return mpr.success;
}

bool Helper_win::executeChangeMtu(const QString &adapter, int mtu)
{
    QMutexLocker locker(&mutex_);

    CMD_CHANGE_MTU cmdChangeMtu;
    cmdChangeMtu.mtu = mtu;
    cmdChangeMtu.storePersistent = false;
    cmdChangeMtu.szAdapterName = adapter.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdChangeMtu;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CHANGE_MTU, stream.str());
    return mpr.success;
}

bool Helper_win::clearDnsOnTap()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CLEAR_DNS_ON_TAP, std::string());
    return mpr.success;
}

QString Helper_win::enableBFE()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_BFE, std::string());
    return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
}

QString Helper_win::resetAndStartRAS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RESET_AND_START_RAS, std::string());
    return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
}

void Helper_win::setIPv6EnabledInFirewall(bool b)
{
    if (bIPV6State_ != b)
    {
        if (!b)
        {
            disableIPv6();
            qCDebug(LOG_BASIC) << "IPv6 disabled";
        }
        else
        {
            enableIPv6();
            qCDebug(LOG_BASIC) << "IPv6 enabled";
        }
        bIPV6State_ = b;
    }
}

void Helper_win::setIPv6EnabledInOS(bool b)
{
    if (!b)
    {
        disableIPv6InOS();
        qCDebug(LOG_BASIC) << "IPv6 in Windows registry disabled";
    }
    else
    {
        enableIPv6InOS();
        qCDebug(LOG_BASIC) << "IPv6 in Windows registry enabled";
    }
}

bool Helper_win::IPv6StateInOS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_OS_IPV6_STATE, std::string());
    return mpr.exitCode;
}

bool Helper_win::addHosts(const QString &hosts)
{
    QMutexLocker locker(&mutex_);
    CMD_ADD_HOSTS cmdAddHosts;
    cmdAddHosts.hosts = hosts.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdAddHosts;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ADD_HOSTS, stream.str());
    return mpr.success;
}

bool Helper_win::removeHosts()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_HOSTS, std::string());
    return mpr.success;
}

void Helper_win::closeAllTcpConnections(bool isKeepLocalSockets)
{
    QMutexLocker locker(&mutex_);
    CMD_CLOSE_TCP_CONNECTIONS cmdCloseTcpConnections;
    cmdCloseTcpConnections.isKeepLocalSockets = isKeepLocalSockets;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdCloseTcpConnections;

    qCDebug(LOG_BASIC) << "Close all active TCP connections (keepLocalSockets = "
                       << cmdCloseTcpConnections.isKeepLocalSockets << ")";
    sendCmdToHelper(AA_COMMAND_CLOSE_TCP_CONNECTIONS, stream.str());
}

QStringList Helper_win::getProcessesList()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENUM_PROCESSES, std::string());
    QStringList list;
    if (mpr.success)
    {
        QString str = QString::fromUtf16((const ushort *)mpr.additionalString.c_str(), mpr.additionalString.size() / sizeof(ushort));

        int pos = 0;
        QString curStr;
        while (pos < str.length())
        {
            if (str.at(pos) == static_cast<QChar>('\0'))
            {
                list << curStr;
                curStr.clear();
            }
            else
            {
                curStr += str[pos];
            }
            pos++;
        }
    }
    return list;
}

bool Helper_win::whitelistPorts(const QString &ports)
{
    QMutexLocker locker(&mutex_);

    CMD_WHITELIST_PORTS cmdWhitelistPorts;
    cmdWhitelistPorts.ports = ports.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdWhitelistPorts;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_WHITELIST_PORTS, stream.str());
    return mpr.success;
}

bool Helper_win::deleteWhitelistPorts()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DELETE_WHITELIST_PORTS, std::string());
    return mpr.success;
}

bool Helper_win::isSupportedICS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_IS_SUPPORTED_ICS, std::string());
    return mpr.exitCode;
}

void Helper_win::enableDnsLeaksProtection()
{
    QMutexLocker locker(&mutex_);

    CMD_DISABLE_DNS_TRAFFIC cmdDisableDnsTraffic;
    if(!customDnsIp_.isEmpty()) {
        cmdDisableDnsTraffic.excludedIps.push_back(customDnsIp_.toStdWString());
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdDisableDnsTraffic;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DISABLE_DNS_TRAFFIC, stream.str());
}

void Helper_win::disableDnsLeaksProtection()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_DNS_TRAFFIC, std::string());
}

bool Helper_win::reinstallWanIkev2()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REINSTALL_WAN_IKEV2, std::string());
    return mpr.exitCode;
}

bool Helper_win::enableWanIkev2()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_WAN_IKEV2, std::string());
    return mpr.exitCode;
}

bool Helper_win::setMacAddressRegistryValueSz(QString subkeyInterfaceName, QString value)
{
    QMutexLocker locker(&mutex_);

    CMD_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ cmdSetMacAddressRegistryValueSz;
    cmdSetMacAddressRegistryValueSz.szInterfaceName = subkeyInterfaceName.toStdWString();
    cmdSetMacAddressRegistryValueSz.szValue = value.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSetMacAddressRegistryValueSz;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_SET_MAC_ADDRESS_REGISTRY_VALUE_SZ, stream.str());
    return mpr.exitCode;
}

bool Helper_win::removeMacAddressRegistryProperty(QString subkeyInterfaceName)
{
    QMutexLocker locker(&mutex_);

    CMD_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY cmdRemoveMacAddressRegistryProperty;
    cmdRemoveMacAddressRegistryProperty.szInterfaceName = subkeyInterfaceName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdRemoveMacAddressRegistryProperty;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_MAC_ADDRESS_REGISTRY_PROPERTY, stream.str());
    return mpr.exitCode;
}

bool Helper_win::resetNetworkAdapter(QString subkeyInterfaceName, bool bringAdapterBackUp)
{
    QMutexLocker locker(&mutex_);

    CMD_RESET_NETWORK_ADAPTER cmdResetNetworkAdapter;
    cmdResetNetworkAdapter.bringBackUp = bringAdapterBackUp;
    cmdResetNetworkAdapter.szInterfaceName = subkeyInterfaceName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdResetNetworkAdapter;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RESET_NETWORK_ADAPTER, stream.str());
    return mpr.exitCode;
}

bool Helper_win::addIKEv2DefaultRoute()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE, std::string());
    return mpr.exitCode;
}

bool Helper_win::removeWindscribeNetworkProfiles()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES, std::string());
    return mpr.exitCode;
}

void Helper_win::setIKEv2IPSecParameters()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_SET_IKEV2_IPSEC_PARAMETERS, std::string());
}

bool Helper_win::makeHostsFileWritable()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_MAKE_HOSTS_FILE_WRITABLE, "");
    if(mpr.success) {
        qCDebug(LOG_BASIC) << "\"hosts\" file is writable now.";
    }
    else {
        qCDebug(LOG_BASIC) << "Was not able to change \"hosts\" file permissions from read-only.";
    }
    return mpr.success;
}

bool Helper_win::reinstallTapDriver(const QString &tapDriverDir)
{
    QMutexLocker locker(&mutex_);

    CMD_REINSTALL_TUN_DRIVER cmdReinstallTunDriver;
    cmdReinstallTunDriver.driverDir.resize(tapDriverDir.size());
    tapDriverDir.toWCharArray(const_cast<wchar_t*>(cmdReinstallTunDriver.driverDir.data()));

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdReinstallTunDriver;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REINSTALL_TAP_DRIVER, stream.str());
    if(mpr.success) {
        qCDebug(LOG_BASIC) << "Tap driver was successfully re-installed.";
    }
    else {
        qCDebug(LOG_BASIC) << "Error to re-install tap driver.";
    }
    return mpr.success;
}

bool Helper_win::reinstallWintunDriver(const QString &wintunDriverDir)
{
    QMutexLocker locker(&mutex_);

    CMD_REINSTALL_TUN_DRIVER cmdReinstallTunDriver;
    cmdReinstallTunDriver.driverDir.resize(wintunDriverDir.size());
    wintunDriverDir.toWCharArray(const_cast<wchar_t*>(cmdReinstallTunDriver.driverDir.data()));

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdReinstallTunDriver;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REINSTALL_WINTUN_DRIVER, stream.str());
    if(mpr.success) {
        qCDebug(LOG_BASIC) << "Wintun driver was successfully re-installed.";
    }
    else {
        qCDebug(LOG_BASIC) << "Error to re-install wintun driver.";
    }
    return mpr.success;
}

void Helper_win::setCustomDnsIp(const QString &ip)
{
    customDnsIp_ = ip;
}

void Helper_win::run()
{
#ifndef DEBUG_DONT_USE_SERVICE
    BIND_CRASH_HANDLER_FOR_THREAD();
    schSCManager_ = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager_ == NULL)
    {
        DWORD err = GetLastError();
        qCDebug(LOG_BASIC) << "OpenSCManager failed: " << err;
        curState_ = STATE_FAILED_CONNECT;
        return;
    }

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    while (schService_ == NULL)
    {
        schService_ = OpenService(schSCManager_, (LPCTSTR)helperLabel_.utf16(), SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (schService_ != NULL)
        {
            break;
        }
        DWORD err = GetLastError();

        Sleep(100);
        if (elapsedTimer.elapsed() > MAX_WAIT_TIME_FOR_HELPER)
        {
            qCDebug(LOG_BASIC) << "OpenService failed: " << err;
            curState_ = STATE_FAILED_CONNECT;
            return;
        }
        if (bStopThread_)
        {
            return;
        }
    }

    bool bStartServiceCalled = false;

    while (true)
    {
        SERVICE_STATUS_PROCESS ssStatus;
        DWORD dwBytesNeeded;

        if (!QueryServiceStatusEx(schService_, SC_STATUS_PROCESS_INFO,
                    (LPBYTE) &ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ))
        {
            qCDebug(LOG_BASIC) << "QueryServiceStatusEx failed: " << GetLastError();
            curState_ = STATE_FAILED_CONNECT;
            return;
        }

        if (ssStatus.dwCurrentState != SERVICE_RUNNING)
        {
            if (!bStartServiceCalled)
            {
                if (StartService(schService_, NULL, NULL) != 0)
                {
                    bStartServiceCalled = true;
                }
            }
        }
        else
        {
            break;
        }
        Sleep(1);
        if (elapsedTimer.elapsed() > MAX_WAIT_TIME_FOR_HELPER)
        {
            qCDebug(LOG_BASIC) << "Timer for QueryServiceStatusEx exceed";
            curState_ = STATE_FAILED_CONNECT;
            return;
        }
        if (bStopThread_)
        {
            return;
        }
    }
#endif
    curState_ = STATE_CONNECTED;
}

MessagePacketResult Helper_win::sendCmdToHelper(int cmdId, const std::string &data)
{
    HANDLE hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        if (WaitNamedPipe(SERVICE_PIPE_NAME, MAX_WAIT_TIME_FOR_PIPE) == 0)
        {
            return MessagePacketResult();
        }

        hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            return MessagePacketResult();
        }
    }

    WinUtils::Win32Handle closePipe(hPipe);

    // first 4 bytes - cmdId
    if (!writeAllToPipe(hPipe, (char *)&cmdId, sizeof(cmdId)))
    {
        return MessagePacketResult();
    }

    // second 4 bytes - size of buffer
    unsigned long sizeOfBuf = data.size();
    if (!writeAllToPipe(hPipe, (char *)&sizeOfBuf, sizeof(sizeOfBuf)))
    {
        return MessagePacketResult();
    }

    // body of message
    if (sizeOfBuf > 0)
    {
        if (!writeAllToPipe(hPipe, data.c_str(), sizeOfBuf))
        {
            return MessagePacketResult();
        }
    }

    // read MessagePacketResult
    MessagePacketResult mpr;
    if (!readAllFromPipe(hPipe, (char *)&sizeOfBuf, sizeof(sizeOfBuf)))
    {
        return mpr;
    }

    if (sizeOfBuf > 0)
    {
        QScopedArrayPointer<char> buf(new char[sizeOfBuf]);
        if (!readAllFromPipe(hPipe, buf.data(), sizeOfBuf))
        {
            return mpr;
        }

        std::istringstream  stream(std::string(buf.data(), sizeOfBuf));
        boost::archive::text_iarchive ia(stream, boost::archive::no_header);

        ia >> mpr;
    }

    return mpr;
}

bool Helper_win::disableIPv6()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_IPV6_DISABLE, std::string());
    return mpr.success;
}

bool Helper_win::enableIPv6()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_IPV6_ENABLE, std::string());
    return mpr.success;
}

bool Helper_win::disableIPv6InOS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_OS_IPV6_DISABLE, std::string());
    return mpr.success;
}

bool Helper_win::enableIPv6InOS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_OS_IPV6_ENABLE, std::string());
    return mpr.success;
}

int Helper_win::debugGetActiveUnblockingCmdCount()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_GET_UNBLOCKING_CMD_COUNT, std::string());
    Q_ASSERT(mpr.exitCode == 0);
    return mpr.exitCode;
}

bool Helper_win::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);

    CMD_FIREWALL_ON cmdFirewallOn;
    cmdFirewallOn.allowLanTraffic = bAllowLanTraffic;
    cmdFirewallOn.ip = ip.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdFirewallOn;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_ON, stream.str());
    return mpr.success;
}

bool Helper_win::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);

    CMD_FIREWALL_ON cmdFirewallOn;
    cmdFirewallOn.allowLanTraffic = bAllowLanTraffic;
    cmdFirewallOn.ip = ip.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdFirewallOn;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_CHANGE, stream.str());
    return mpr.success;
}

bool Helper_win::firewallOff()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_OFF, std::string());
    return mpr.success;
}

bool Helper_win::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_STATUS, std::string());
    Q_ASSERT(mpr.success);
    return mpr.exitCode == 1;
}

void Helper_win::initVariables()
{
    if (schService_)
    {
        CloseServiceHandle(schService_);
        schService_ = NULL;
    }
    if (schSCManager_)
    {
        CloseServiceHandle(schSCManager_);
        schSCManager_ = NULL;
    }
    curState_= STATE_INIT;
    bStopThread_ = false;
    bIPV6State_ = true;
}

bool Helper_win::readAllFromPipe(HANDLE hPipe, char *buf, DWORD len)
{
    char *ptr = buf;
    DWORD dwRead = 0;
    DWORD lenCopy = len;
    while (lenCopy > 0)
    {
        if (::ReadFile(hPipe, ptr, lenCopy, &dwRead, 0))
        {
            ptr += dwRead;
            lenCopy -= dwRead;
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool Helper_win::writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len)
{
    const char *ptr = buf;
    DWORD dwWrite = 0;
    while (len > 0)
    {
        if (::WriteFile(hPipe, ptr, len, &dwWrite, 0))
        {
            ptr += dwWrite;
            len -= dwWrite;
        }
        else
        {
            return false;
        }
    }
    return true;
}
