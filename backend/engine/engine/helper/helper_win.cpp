#include "utils/boost_includes.h"
#include "utils/executable_signature/executable_signature.h"
#include "helper_win.h"
#include "utils/logger.h"
#include <QDir>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <sstream>
#include "windscribeinstallhelper_win.h"
#include "engine/openvpnversioncontroller.h"
#include "simple_xor_crypt.h"


#define SERVICE_PIPE_NAME  (L"\\\\.\\pipe\\WindscribeService")

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
    if (isHelperConnected())
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
    if (!ExecutableSignature::verify(serviceExePath))
    {
        qCDebug(LOG_BASIC) << "WindscribeService signature incorrect";
        bFailedConnectToHelper_ = true;
        return;
    }

    start(QThread::LowPriority);
}

QString Helper_win::executeRootCommand(const QString &/*commandLine*/)
{
    Q_ASSERT(false);
    return "";
}

bool Helper_win::executeRootUnblockingCommand(const QString &/*commandLine*/, unsigned long &/*outCmdId*/, const QString &/*eventName*/)
{
    Q_ASSERT(false);
    return false;
}

bool Helper_win::executeOpenVPN(const QString &/*commandLine*/, const QString &/*pathToOvpnConfig*/, unsigned long &/*outCmdId*/)
{
    Q_ASSERT(false);
    return false;
}

bool Helper_win::executeOpenVPN(const QString &configPath, unsigned int portNumber, const QString &httpProxy, unsigned int httpPort, const QString &socksProxy, unsigned int socksPort, unsigned long &outCmdId)
{
    QMutexLocker locker(&mutex_);

    // check openvpn executable signature
    QString openVpnExePath = QCoreApplication::applicationDirPath() + "/" + OpenVpnVersionController::instance().getSelectedOpenVpnExecutable();
    if (!ExecutableSignature::verify(openVpnExePath))
    {
        qCDebug(LOG_CONNECTION) << "OpenVPN executable signature incorrect";
        return false;
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
    mpr.clear();

    return mpr.success;
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
    mpr.clear();

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
    mpr.clear();

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
    QString answer = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    mpr.clear();

    return answer;
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
    QString answer = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    mpr.clear();

    return answer;
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
    QString answer = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    mpr.clear();

    return answer;
}

bool Helper_win::executeChangeIcs(int cmd, const QString &configPath, const QString &publicGuid, const QString &privateGuid, unsigned long &outCmdId, const QString &eventName)
{
    QMutexLocker locker(&mutex_);

    // check executable signature
    QString updateIcsExePath = QCoreApplication::applicationDirPath() + "/ChangeIcs.exe";
    if (!ExecutableSignature::verify(updateIcsExePath))
    {
        qCDebug(LOG_BASIC) << "ChangeIcs.exe incorrect signature";
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
    mpr.clear();

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
    mpr.clear();

    return mpr.success;
}

QString Helper_win::executeUpdateInstaller(const QString &installerPath, unsigned long waitingOnPid, bool &success)
{
    QMutexLocker locker(&mutex_);

    CMD_RUN_UPDATE_INSTALLER cmdRunUpdateInstaller;
    cmdRunUpdateInstaller.szUpdateInstallerLocation = installerPath.toStdWString();
    cmdRunUpdateInstaller.waitingOnPid = waitingOnPid;

    std::stringstream stream;
    boost::archive::text_oarchive oa(stream, boost::archive::no_header);
    oa << cmdRunUpdateInstaller;

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RUN_UPDATE_INSTALLER, stream.str());

    QString errorOutput;
    if (!mpr.success)
    {
        errorOutput = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    }
    success = mpr.success;
    mpr.clear();

    return errorOutput;
}

bool Helper_win::clearDnsOnTap()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_CLEAR_DNS_ON_TAP, std::string());
    mpr.clear();

    return mpr.success;
}

QString Helper_win::enableBFE()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_BFE, std::string());
    QString answer = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    mpr.clear();

    return answer;
}

QString Helper_win::resetAndStartRAS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_RESET_AND_START_RAS, std::string());
    QString answer = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
    mpr.clear();

    return answer;
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
    mpr.clear();
    return mpr.exitCode;
}

bool Helper_win::isHelperConnected()
{
    return bHelperConnected_;
}

bool Helper_win::isFailedConnectToHelper()
{
    return bFailedConnectToHelper_;
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
        QString str = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
        mpr.clear();
        return str;
    }
    else
    {
        return "failed detect";
    }
}

void Helper_win::enableMacSpoofingOnBoot(bool /*bEnable*/, QString /*interfaceName*/, QString /*macAddress*/)
{
    // do nothing on windows
}

void Helper_win::enableFirewallOnBoot(bool bEnable)
{
    Q_UNUSED(bEnable);
}

bool Helper_win::removeWindscribeUrlsFromHosts()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_WINDSCRIBE_FROM_HOSTS, std::string());
    mpr.clear();
    return mpr.success;
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
    mpr.clear();
    return mpr.success;
}

bool Helper_win::removeHosts()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_HOSTS, std::string());
    mpr.clear();
    return mpr.success;
}

bool Helper_win::reinstallHelper()
{
    QString servicePath = QCoreApplication::applicationDirPath() + "/WindscribeService.exe";
    QString subinaclPath = QCoreApplication::applicationDirPath() + "/subinacl.exe";
    return WindscribeInstallHelper_win::executeInstallHelperCmd(servicePath, subinaclPath);
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
        QString str = QString::fromUtf16((const ushort *)mpr.szAdditionalData, mpr.sizeOfAdditionalData / sizeof(ushort));

        int pos = 0;
        QString curStr;
        while (pos < str.length())
        {
            if (str[pos] == '\0')
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

        mpr.clear();
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
    mpr.clear();
    return mpr.success;
}

bool Helper_win::deleteWhitelistPorts()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DELETE_WHITELIST_PORTS, std::string());
    mpr.clear();
    return mpr.success;
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
            outLog = QString::fromLocal8Bit((const char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData);
        }
        mpr.clear();
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

bool Helper_win::isSupportedICS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_IS_SUPPORTED_ICS, std::string());
    mpr.clear();
    return mpr.exitCode;
}

QStringList Helper_win::getActiveNetworkInterfaces_mac()
{
    //nothing todo for Windows
    return QStringList();
}

bool Helper_win::setKeychainUsernamePassword(const QString &username, const QString &password)
{
    Q_UNUSED(username);
    Q_UNUSED(password);
    //nothing todo for Windows
    return false;
}

void Helper_win::enableDnsLeaksProtection()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_DISABLE_DNS_TRAFFIC, std::string());
    mpr.clear();
}

void Helper_win::disableDnsLeaksProtection()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_DNS_TRAFFIC, std::string());
    mpr.clear();
}

bool Helper_win::reinstallWanIkev2()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REINSTALL_WAN_IKEV2, std::string());
    mpr.clear();
    return mpr.exitCode;
}

bool Helper_win::enableWanIkev2()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ENABLE_WAN_IKEV2, std::string());
    mpr.clear();
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
    mpr.clear();
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
    mpr.clear();
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
    mpr.clear();
    return mpr.exitCode;
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
    mpr.clear();
    return mpr.exitCode;
}

bool Helper_win::addIKEv2DefaultRoute()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_ADD_IKEV2_DEFAULT_ROUTE, std::string());
    mpr.clear();
    return mpr.exitCode;
}

bool Helper_win::removeWindscribeNetworkProfiles()
{
    QMutexLocker locker(&mutex_);
    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_REMOVE_WINDSCRIBE_NETWORK_PROFILES, std::string());
    mpr.clear();
    return mpr.exitCode;
}

void Helper_win::sendConnectStatus(bool /*isConnected*/, const SplitTunnelingNetworkInfo &/*stni*/)
{
    // nothing todo for Windows
}

bool Helper_win::setKextPath(const QString &/*kextPath*/)
{
    // nothing todo for Windows
    return true;
}

void Helper_win::run()
{
    schSCManager_ = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager_ == NULL)
    {
        DWORD err = GetLastError();
        qCDebug(LOG_BASIC) << "OpenSCManager failed: " << err;
        bFailedConnectToHelper_ = true;
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
            bFailedConnectToHelper_ = true;
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
            bFailedConnectToHelper_ = true;
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
            bFailedConnectToHelper_ = true;
            return;
        }
        if (bStopThread_)
        {
            return;
        }
    }

    bHelperConnected_ = true;
}

MessagePacketResult Helper_win::sendCmdToHelper(int cmdId, const std::string &data)
{
    MessagePacketResult mpr;

    HANDLE hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        if (WaitNamedPipe(SERVICE_PIPE_NAME, MAX_WAIT_TIME_FOR_PIPE) == 0)
        {
            return mpr;
        }
        else
        {
            hPipe = ::CreateFileW(SERVICE_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
            if (hPipe == INVALID_HANDLE_VALUE)
            {
                return mpr;
            }
        }
    }

    // first 4 bytes - cmdId
    if (!writeAllToPipe(hPipe, (char *)&cmdId, sizeof(cmdId)))
    {
        CloseHandle(hPipe);
        return mpr;
    }

    // second 4 bytes - pid
    unsigned long pid = GetCurrentProcessId();
    if (!writeAllToPipe(hPipe, (char *)&pid, sizeof(pid)))
    {
        CloseHandle(hPipe);
        return mpr;
    }

    // third 4 bytes - size of buffer
    unsigned long sizeOfBuf = data.size();
    if (!writeAllToPipe(hPipe, (char *)&sizeOfBuf, sizeof(sizeOfBuf)))
    {
        CloseHandle(hPipe);
        return mpr;
    }

    // body of message
    if (sizeOfBuf > 0)
    {
        if (!writeAllToPipe(hPipe, data.c_str(), sizeOfBuf))
        {
            CloseHandle(hPipe);
            return mpr;
        }
    }

    if (!readAllFromPipe(hPipe, (char *)&mpr, sizeof(mpr) - sizeof(mpr.sizeOfAdditionalData)))
    {
        CloseHandle(hPipe);
        mpr.success = false;
        return mpr;
    }

    if (mpr.sizeOfAdditionalData > 0)
    {
        mpr.szAdditionalData = new char[mpr.sizeOfAdditionalData];
        if (!readAllFromPipe(hPipe, (char *)mpr.szAdditionalData, mpr.sizeOfAdditionalData))
        {
            CloseHandle(hPipe);
            mpr.success = false;
            return mpr;
        }
    }

    CloseHandle(hPipe);

    return mpr;
}

bool Helper_win::disableIPv6()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_IPV6_DISABLE, std::string());
    mpr.clear();
    return mpr.success;
}

bool Helper_win::enableIPv6()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_FIREWALL_IPV6_ENABLE, std::string());
    mpr.clear();
    return mpr.success;
}

bool Helper_win::disableIPv6InOS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_OS_IPV6_DISABLE, std::string());
    mpr.clear();
    return mpr.success;
}

bool Helper_win::enableIPv6InOS()
{
    QMutexLocker locker(&mutex_);

    MessagePacketResult mpr = sendCmdToHelper(AA_COMMAND_OS_IPV6_ENABLE, std::string());
    mpr.clear();
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
    bFailedConnectToHelper_ = false;
    bHelperConnected_ = false;
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

    std::string s(buf, len);
    std::string d = SimpleXorCrypt::decrypt(s, ENCRYPT_KEY);
    memcpy(buf, d.data(), len);

    return true;
}

bool Helper_win::writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len)
{
    std::string src(buf, len);
    std::string encodedSrc = SimpleXorCrypt::encrypt(src, ENCRYPT_KEY);

    const char *ptr = encodedSrc.data();
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
