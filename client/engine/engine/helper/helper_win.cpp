#include "helper_win.h"

#include <QCoreApplication>
#include <QDir>
#include <QScopeGuard>

#include <sstream>

#include "utils/boost_includes.h"

#include "../../../../backend/windows/windscribe_service/ipc/serialize_structs.h"
#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "engine/openvpnversioncontroller.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "installhelper_win.h"
#include "types/wireguardtypes.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/logger.h"
#include "utils/ws_assert.h"
#include "utils/winutils.h"

#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

//  the program to connect to the helper socket without starting the service (uncomment for debug purpose)
// #define DEBUG_DONT_USE_SERVICE

Helper_win::Helper_win(QObject *parent) : IHelper(parent)
{
    initVariables();
}

Helper_win::~Helper_win()
{
    scm_.blockStartStopRequests();
    wait();

#if defined QT_DEBUG
    if (curState_ == STATE_CONNECTED)
    {
        debugGetActiveUnblockingCmdCount();
        qCDebug(LOG_BASIC) << "Active unblocking commands in helper on exit:" << debugGetActiveUnblockingCmdCount();
    }
#endif
}

void Helper_win::startInstallHelper()
{
    initVariables();

    QString serviceExePath = QCoreApplication::applicationDirPath() + "/WindscribeService.exe";
    ExecutableSignature sigCheck;
    if (!sigCheck.verify(serviceExePath.toStdWString())) {
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
    return InstallHelper_win::executeInstallHelperCmd();
}

void Helper_win::setNeedFinish()
{
    //nothing todo for Windows
}

QString Helper_win::getHelperVersion()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_GET_HELPER_VERSION, std::string());
    if (mpr.success) {
        return QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
    }

    return "failed detect";
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

    if (mpr.success) {
        outFinished = mpr.blockingCmdFinished;
        if (outFinished) {
            outLog = QString::fromLocal8Bit(mpr.additionalString.c_str(), mpr.additionalString.size());
        }
    }
    else {
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

bool Helper_win::setSplitTunnelingSettings(bool isActive, bool isExclude, bool isAllowLanTraffic,
                                           const QStringList &files, const QStringList &ips,
                                           const QStringList &hosts)
{
    QMutexLocker locker(&mutex_);

    CMD_SPLIT_TUNNELING_SETTINGS cmdSplitTunnelingSettings;
    cmdSplitTunnelingSettings.isActive = isActive;
    cmdSplitTunnelingSettings.isExclude = isExclude;
    cmdSplitTunnelingSettings.isAllowLanTraffic = isAllowLanTraffic;

    for (int i = 0; i < files.count(); ++i) {
        cmdSplitTunnelingSettings.files.push_back(files[i].toStdWString());
    }

    for (int i = 0; i < ips.count(); ++i) {
        cmdSplitTunnelingSettings.ips.push_back(ips[i].toStdWString());
    }

    for (int i = 0; i < hosts.count(); ++i) {
        cmdSplitTunnelingSettings.hosts.push_back(hosts[i].toStdString());
    }

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdSplitTunnelingSettings;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_SPLIT_TUNNELING_SETTINGS, stream.str());
    return mpr.exitCode;
}

bool Helper_win::sendConnectStatus(bool isConnected, bool isTerminateSocket, bool isKeepLocalSocket,
                                   const AdapterGatewayInfo &defaultAdapter, const AdapterGatewayInfo &vpnAdapter,
                                   const QString &connectedIp, const types::Protocol &protocol)
{
    QMutexLocker locker(&mutex_);

    CMD_CONNECT_STATUS cmd;
    cmd.isConnected = isConnected;
    cmd.isTerminateSocket = isTerminateSocket;
    cmd.isKeepLocalSocket = isKeepLocalSocket;

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

    if (isConnected) {
        if (protocol.isStunnelOrWStunnelProtocol()) {
            cmd.protocol = CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL;
        }
        else if (protocol.isIkev2Protocol()) {
            cmd.protocol = CMD_PROTOCOL_IKEV2;
        }
        else if (protocol.isWireGuardProtocol()) {
            cmd.protocol = CMD_PROTOCOL_WIREGUARD;
        }
        else if (protocol.isOpenVpnProtocol()) {
            cmd.protocol = CMD_PROTOCOL_OPENVPN;
        }
        else {
            WS_ASSERT(false);
        }

        fillAdapterInfo(vpnAdapter, cmd.vpnAdapter);

        cmd.connectedIp = connectedIp.toStdString();
        cmd.remoteIp = vpnAdapter.remoteIp().toStdString();
    }

    fillAdapterInfo(defaultAdapter, cmd.defaultAdapter);

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CONNECT_STATUS, stream.str());
    return mpr.success;
}

bool Helper_win::setCustomDnsWhileConnected(unsigned long ifIndex, const QString &overrideDnsIpAddress)
{
    QMutexLocker locker(&mutex_);

    CMD_CONNECTED_DNS cmd;
    cmd.ifIndex = ifIndex;
    cmd.szDnsIpAddress = overrideDnsIpAddress.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CONNECTED_DNS, stream.str());
    return mpr.exitCode == 0;
}

IHelper::ExecuteError Helper_win::startWireGuard()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_START_WIREGUARD, std::string());
    return mpr.success ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_win::stopWireGuard()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_STOP_WIREGUARD, std::string());
    return mpr.success;
}

bool Helper_win::configureWireGuard(const WireGuardConfig &config)
{
    QMutexLocker locker(&mutex_);

    CMD_CONFIGURE_WIREGUARD cmd;
    cmd.config = config.generateConfigFile().toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmd;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CONFIGURE_WIREGUARD, stream.str());
    return mpr.success ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_win::getWireGuardStatus(types::WireGuardStatus *status)
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_GET_WIREGUARD_STATUS, std::string());
    if (mpr.success && status) {
        if (mpr.exitCode == WIREGUARD_STATE_ERROR) {
            status->state = types::WireGuardState::FAILURE;
            status->lastHandshake = 0;
            status->bytesReceived = 0;
            status->bytesTransmitted = 0;
        }
        else {
            status->state = types::WireGuardState::ACTIVE;
            status->lastHandshake = mpr.customInfoValue[0];
            status->bytesTransmitted = mpr.customInfoValue[1];
            status->bytesReceived = mpr.customInfoValue[2];
        }
    }

    return mpr.success;
}

bool Helper_win::startCtrld(const QString &upstream1, const QString &upstream2, const QStringList &domains, bool isCreateLog)
{
    // Nothing to do.
    return true;
}

bool Helper_win::stopCtrld()
{
    // Nothing to do.
    return true;
}

bool Helper_win::isHelperConnected() const
{
    return curState_ == STATE_CONNECTED;
}

IHelper::ExecuteError Helper_win::executeOpenVPN(const QString &config, unsigned int portNumber, const QString &httpProxy,
                                                 unsigned int httpPort, const QString &socksProxy, unsigned int socksPort,
                                                 unsigned long &outCmdId, bool isCustomConfig)
{
    QMutexLocker locker(&mutex_);

    CMD_RUN_OPENVPN cmdRunOpenVpn;
    cmdRunOpenVpn.szConfig = config.toStdWString();
    cmdRunOpenVpn.portNumber = portNumber;
    cmdRunOpenVpn.szHttpProxy = httpProxy.toStdWString();
    cmdRunOpenVpn.szSocksProxy = socksProxy.toStdWString();
    cmdRunOpenVpn.httpPortNumber = httpPort;
    cmdRunOpenVpn.socksPortNumber = socksPort;
    cmdRunOpenVpn.isCustomConfig = isCustomConfig;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdRunOpenVpn;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RUN_OPENVPN, stream.str());
    outCmdId = mpr.blockingCmdId;

    return mpr.success ? IHelper::EXECUTE_SUCCESS : IHelper::EXECUTE_ERROR;
}

bool Helper_win::executeTaskKill(CmdKillTarget target)
{
    QMutexLocker locker(&mutex_);

    CMD_TASK_KILL cmdTaskKill;
    cmdTaskKill.target = target;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdTaskKill;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_TASK_KILL, stream.str());

    if (mpr.success && mpr.exitCode != 0) {
        QString result = QString::fromStdString(mpr.additionalString).trimmed();
        if (!result.startsWith("ERROR: The process") && !result.endsWith("not found.")) {
            qCDebug(LOG_BASIC) << QString("executeTaskKill(%1) failed: %2").arg(target).arg(result);
        }
    }

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

bool Helper_win::changeIcs(const QString &adapterName)
{
    QMutexLocker locker(&mutex_);
    CMD_ICS_CHANGE cmdIcsChange;
    cmdIcsChange.szAdapterName = adapterName.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdIcsChange;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ICS_CHANGE, stream.str());
    return mpr.success;
}

bool Helper_win::stopIcs()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ICS_STOP, std::string());
    return mpr.exitCode;
}

bool Helper_win::changeMtu(const QString &adapter, int mtu)
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
    if (bIPV6State_ != b) {
        if (!b) {
            disableIPv6();
            qCDebug(LOG_BASIC) << "IPv6 disabled";
        }
        else {
            enableIPv6();
            qCDebug(LOG_BASIC) << "IPv6 enabled";
        }
        bIPV6State_ = b;
    }
}

void Helper_win::setIPv6EnabledInOS(bool b)
{
    if (!b) {
        disableIPv6InOS();
        qCDebug(LOG_BASIC) << "IPv6 in Windows registry disabled";
    }
    else {
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
    if (mpr.success) {
        QString str = QString::fromUtf16((const char16_t *)mpr.additionalString.c_str(), mpr.additionalString.size() / sizeof(char16_t));

        int pos = 0;
        QString curStr;
        while (pos < str.length()) {
            if (str.at(pos) == static_cast<QChar>('\0')) {
                list << curStr;
                curStr.clear();
            }
            else {
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

bool Helper_win::isIcsSupported()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ICS_IS_SUPPORTED, std::string());
    return mpr.exitCode;
}

bool Helper_win::startIcs()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ICS_START, std::string());
    return mpr.exitCode;
}

void Helper_win::enableDnsLeaksProtection()
{
    QMutexLocker locker(&mutex_);

    CMD_DISABLE_DNS_TRAFFIC cmdDisableDnsTraffic;

    for (const auto &ip : customDnsIp_)
        cmdDisableDnsTraffic.excludedIps.push_back(ip.toStdWString());

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
    if (mpr.success) {
        qCDebug(LOG_BASIC) << "\"hosts\" file is writable now.";
    }
    else {
        qCDebug(LOG_BASIC) << "Was not able to change \"hosts\" file permissions from read-only.";
    }
    return mpr.success;
}

void Helper_win::setCustomDnsIps(const QStringList &ips)
{
    customDnsIp_ = ips;
}

void Helper_win::run()
{
#ifndef DEBUG_DONT_USE_SERVICE
    try {
        scm_.openSCM(SC_MANAGER_CONNECT);
        scm_.openService(L"WindscribeService", SERVICE_QUERY_STATUS | SERVICE_START);
        auto status = scm_.queryServiceStatus();
        if (status != SERVICE_RUNNING) {
            scm_.startService();
        }
        curState_ = STATE_CONNECTED;
    }
    catch (std::system_error& ex) {
        qCDebug(LOG_BASIC) << "Helper_win::run -" << ex.what();
        curState_ = STATE_FAILED_CONNECT;
    }

    scm_.closeSCM();
#else
    curState_ = STATE_CONNECTED;
#endif
}

MessagePacketResult Helper_win::sendCmdToHelper(int cmdId, const std::string &data)
{
    if (helperPipe_.isValid()) {
        // Check if our IPC connection has become invalid (e.g. the helper is restarted while the app is running).
        DWORD flags;
        BOOL result = ::GetNamedPipeInfo(helperPipe_.getHandle(), &flags, NULL, NULL, NULL);
        if (result == FALSE) {
            qCDebug(LOG_BASIC) << "Reconnecting helper pipe as existing handle instance is invalid" << ::GetLastError();
            helperPipe_.closeHandle();
        }
    }

    if (!helperPipe_.isValid()) {
        HANDLE hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE) {
            if (::WaitNamedPipe(SERVICE_PIPE_NAME, MAX_WAIT_TIME_FOR_PIPE) == FALSE) {
                return MessagePacketResult();
            }

            hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
            if (hPipe == INVALID_HANDLE_VALUE) {
                return MessagePacketResult();
            }
        }

        helperPipe_.setHandle(hPipe);
    }

    auto closePipe = qScopeGuard([&] {
        // Close the pipe if any errors occur.
        helperPipe_.closeHandle();
    });

    // first 4 bytes - cmdId
    if (!writeAllToPipe(helperPipe_.getHandle(), (char *)&cmdId, sizeof(cmdId))) {
        return MessagePacketResult();
    }

    // second 4 bytes - size of buffer
    unsigned long sizeOfBuf = data.size();
    if (!writeAllToPipe(helperPipe_.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
        return MessagePacketResult();
    }

    // body of message
    if (sizeOfBuf > 0) {
        if (!writeAllToPipe(helperPipe_.getHandle(), data.c_str(), sizeOfBuf)) {
            return MessagePacketResult();
        }
    }

    // read MessagePacketResult
    MessagePacketResult mpr;
    if (!readAllFromPipe(helperPipe_.getHandle(), (char *)&sizeOfBuf, sizeof(sizeOfBuf))) {
        return mpr;
    }

    if (sizeOfBuf > 0) {
        QScopedArrayPointer<char> buf(new char[sizeOfBuf]);
        if (!readAllFromPipe(helperPipe_.getHandle(), buf.data(), sizeOfBuf)) {
            return mpr;
        }

        std::istringstream stream(std::string(buf.data(), sizeOfBuf));
        boost::archive::text_iarchive ia(stream, boost::archive::no_header);

        ia >> mpr;
    }

    closePipe.dismiss();

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
    WS_ASSERT(mpr.exitCode == 0);
    return mpr.exitCode;
}

bool Helper_win::firewallOn(const QString &connectingIp, const QString &ip, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    QMutexLocker locker(&mutex_);

    CMD_FIREWALL_ON cmdFirewallOn;
    cmdFirewallOn.connectingIp = connectingIp.toStdWString();
    cmdFirewallOn.allowLanTraffic = bAllowLanTraffic;
    cmdFirewallOn.isCustomConfig = bIsCustomConfig;
    cmdFirewallOn.ip = ip.toStdWString();

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdFirewallOn;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_ON, stream.str());
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
    WS_ASSERT(mpr.success);
    return mpr.exitCode == 1;
}

void Helper_win::initVariables()
{
    curState_= STATE_INIT;
    bIPV6State_ = true;
    scm_.unblockStartStopRequests();
}

bool Helper_win::readAllFromPipe(HANDLE hPipe, char *buf, DWORD len)
{
    char *ptr = buf;
    DWORD dwRead = 0;
    DWORD lenCopy = len;
    while (lenCopy > 0) {
        if (::ReadFile(hPipe, ptr, lenCopy, &dwRead, 0)) {
            ptr += dwRead;
            lenCopy -= dwRead;
        }
        else {
            return false;
        }
    }

    return true;
}

bool Helper_win::writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len)
{
    const char *ptr = buf;
    DWORD dwWrite = 0;
    while (len > 0) {
        if (::WriteFile(hPipe, ptr, len, &dwWrite, 0)) {
            ptr += dwWrite;
            len -= dwWrite;
        }
        else {
            return false;
        }
    }
    return true;
}

bool Helper_win::createOpenVpnAdapter(bool useDCODriver)
{
    QMutexLocker locker(&mutex_);

    CMD_CREATE_OPENVPN_ADAPTER cmdCreateAdapter;
    cmdCreateAdapter.useDCODriver = useDCODriver;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdCreateAdapter;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CREATE_OPENVPN_ADAPTER, stream.str());
    return mpr.exitCode;
}

bool Helper_win::removeOpenVpnAdapter()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_OPENVPN_ADAPTER, std::string());
    return mpr.success;
}

bool Helper_win::disableDohSettings()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DISABLE_DOH_SETTINGS, std::string());
    return mpr.success;
}

bool Helper_win::enableDohSettings()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_DOH_SETTINGS, std::string());
    return mpr.success;
}
